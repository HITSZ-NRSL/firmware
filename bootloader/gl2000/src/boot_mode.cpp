#include "hw_config.h"
#include "boot_mode.h"
#include "params_hal.h"
#include "ui_hal.h"
#include "flash_map.h"
#include "system_config.h"
#include "esp8266_comm.h"
#include "sx1278_comm.h"
#include "boot_debug.h"

#define UPDATE_BLINK_PERIOD 100

extern UART_HandleTypeDef UartHandleEsp8266;
extern SDK_QUEUE USART_Esp8266_Queue;

pFunction JumpToApplication;
uint32_t JumpAddress;

void start_app(void)
{
    if(((*(__IO uint32_t*)CORE_FW_ADDRESS) & APP_START_MASK ) == 0x20000000) {
        /* Jump to user application */
        JumpAddress = *(__IO uint32_t*) (CORE_FW_ADDRESS + 4);
        JumpToApplication = (pFunction) JumpAddress;

        /* Initialize user application's Stack Pointer */
        __set_MSP(*(__IO uint32_t*) CORE_FW_ADDRESS);
        JumpToApplication();
    }
}

void USBD_DFU_Init(void)
{
    USBD_Init(&USBD_Device, &DFU_Desc, 0);
    USBD_RegisterClass(&USBD_Device, USBD_DFU_CLASS);
    USBD_DFU_RegisterMedia(&USBD_Device, &USBD_DFU_fops);
    USBD_Start(&USBD_Device);
}

void USBD_CDC_Init(void)
{
    USBD_Init(&USBD_Device, &VCP_Desc, 0);
    USBD_RegisterClass(&USBD_Device, USBD_CDC_CLASS);
    USBD_CDC_RegisterInterface(&USBD_Device, &USBD_CDC_fops);
    USBD_Start(&USBD_Device);
}

void USBD_CDC_Process(void)
{
    uint8_t TxBuffer[1024];
    uint32_t len=0;

    len=0;
    while(!sdkIsQueueEmpty(&USART_Esp8266_Queue)) {
        if(len < sizeof(TxBuffer)) {
            sdkGetQueueData(&USART_Esp8266_Queue, &TxBuffer[len++]);
        } else {
            break;
        }
    }

    if(len) {
        if (USBD_STATE_CONFIGURED == USBD_Device.dev_state) {
            USBD_CDC_SetTxBuffer(&USBD_Device, TxBuffer, len);
            while(USBD_CDC_TransmitPacket(&USBD_Device) != USBD_OK);
        }
    }

    len=0;
    while(!sdkIsQueueEmpty(&USB_Rx_Queue)) {
        if(len < sizeof(TxBuffer)) {
            sdkGetQueueData(&USB_Rx_Queue, &TxBuffer[len++]);
        } else {
            break;
        }
    }
    if(len) {
        HAL_UART_Transmit(&UartHandleEsp8266, TxBuffer, len, 2000);//2000ms
    }
}

bool FLASH_Restore(Firmware_TypeDef FmType)
{
    bool result;
    uint32_t size;

    if(OTA_FIRWARE == FmType) {
        size = HAL_PARAMS_Get_Boot_ota_app_size();
        if(0 == size) {
            return true;
        }
        result = FLASH_CopyMemory(FLASH_SERIAL, EXTERNAL_FLASH_OTA_ADDRESS, FLASH_INTERNAL, CORE_FW_ADDRESS, size, 0, 0);
    } else {
        return true;
    }
    return result;
}

bool DEFAULT_Flash_Reset(void)
{
    return FLASH_Restore(DEFAULT_FIRWARE);
}

bool OTA_Flash_Reset(void)
{
    return FLASH_Restore(OTA_FIRWARE);
}

void Enter_Serail_Com_Mode(void)
{
    HAL_UI_UserLED_Control(1);
    HAL_PARAMS_Set_Boot_boot_flag(BOOT_FLAG_NORMAL);
    HAL_PARAMS_Save_Params();
    USBD_CDC_Init();
    while (1) {
        USBD_CDC_Process();
    }
}

void Enter_Factory_RESTORE_Mode(void)
{
    HAL_UI_UserLED_Control(1);
    HAL_PARAMS_Set_Boot_initparam_flag(INITPARAM_FLAG_FACTORY_RESET);
    HAL_PARAMS_Set_Boot_boot_flag(BOOT_FLAG_NORMAL);
    HAL_PARAMS_Save_Params();
}

void Enter_OTA_Update_Mode(void)
{
    HAL_UI_UserLED_Control(1);
    if(OTA_Flash_Reset()) {
        HAL_PARAMS_Set_Boot_boot_flag(BOOT_FLAG_NORMAL);
        HAL_PARAMS_Save_Params();
    } else {
        System_Reset();
    }
}

void Enter_DFU_Mode(void)
{
    HAL_UI_UserLED_Control(1);
    USBD_DFU_Init();
    while(1)
    {}
}

void Enter_ESP8266_Update_Mode(void)
{
    HAL_UI_UserLED_Control(0);
    Esp8266_Enter_UpdateMode();
    while(1)
    {}
}

void Enter_Flash_Test(void)
{
    sFLASH_Init();
    uint32_t success_count = 0, failed_count = 0;
    int result =0;
    while(1) {
        result = sFLASH_SelfTest();
        if(0 == result) {
            success_count ++;
        } else {
            failed_count ++;
        }
        //BOOT_DEBUG("success_count = %d, failed_count = %d\r\n", success_count, failed_count);
        delay(1000);
    }
}

static void USB_USART_Send_Data(uint8_t Data)
{
    volatile system_tick_t start_micros, current_micros, elapsed_micros;

    start_micros = micros();
    if (USBD_STATE_CONFIGURED == USBD_Device.dev_state) {
        USBD_CDC_SetTxBuffer(&USBD_Device, &Data, 1);
        while(USBD_CDC_TransmitPacket(&USBD_Device) != USBD_OK) {
            current_micros = micros();
            elapsed_micros = current_micros - start_micros;

            if (elapsed_micros < 0){
                elapsed_micros =  0xFFFFFFFF - start_micros + current_micros;
            }

            if (elapsed_micros >= 2000) {  //2000us 超时
                break;
            }
        }
    }
}

void error_output(const char* msg)
{
    while(*msg) {
        USB_USART_Send_Data((uint8_t)(*msg));
        msg++;
    }
}

void Enter_Self_Check(void)
{
    BOOT_DEBUG("Enter_Self_Check\r\n");
    int count = 0, flag = 0;

    if(!ESP8266_SelfCheck()) {
        flag = 1;
    }

    if(!flag && !SX1278_SelfCheck()) {
        flag = 2;
    }

    if(!flag && !sFLASH_SelfCheck()) {
        flag = 3;
    }

    if(flag) {
        USBD_CDC_Init();
        while(1) {
            HAL_UI_TrLED_Control(0);
            HAL_UI_UserLED_Control(0);
            delay(400);
            HAL_UI_TrLED_Control(1);
            HAL_UI_UserLED_Control(1);
            delay(400);
            if(count++ > 1) {
                switch(flag) {
                    case 1:
                        error_output("ESP8266 check fail!!!\r\n");
                        break;
                    case 2:
                        error_output("sx1278 check fail!!!\r\n");
                        break;
                    case 3:
                        error_output("spi flash check fail!!!\r\n");
                        break;
                    default:
                        break;
                }
                count = 0;
            }
        }
    }
}

