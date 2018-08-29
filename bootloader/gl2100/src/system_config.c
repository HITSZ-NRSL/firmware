#include "hw_config.h"
#include "rtc_hal.h"
#include "ui_hal.h"
#include "boot_debug.h"


#define ESP8266_EN_GPIO_PIN              GPIO_PIN_9
#define ESP8266_EN_GPIO_PORT             GPIOA
#define ESP8266_RST_GPIO_PIN             GPIO_PIN_10
#define ESP8266_RST_GPIO_PORT            GPIOA
#define SIM800C_PWR_EN_GPIO_PIN          GPIO_PIN_8
#define SIM800C_PWR_EN_GPIO_PORT         GPIOB
#define SIM800C_PWR_KEY_GPIO_PIN         GPIO_PIN_13
#define SIM800C_PWR_KEY_GPIO_PORT        GPIOC

#define CELLULAR_USART_QUEUE_SIZE              1024

UART_HandleTypeDef UartHandleDebug;
UART_HandleTypeDef UartHandleCellular;

SDK_QUEUE USART_Cellular_Queue;

void log_output(const char* msg)
{
    while(*msg)
    {
        HAL_UART_Transmit(&UartHandleDebug, (uint8_t *)msg++, 1, 10);
    }
}

void usart_debug_initial(uint32_t baud)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();

    GPIO_InitTypeDef  GPIO_InitStruct;
    /* UART TX GPIO pin configuration  */
    GPIO_InitStruct.Pin       = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FAST;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    UartHandleDebug.Instance          = USART2;
    UartHandleDebug.Init.BaudRate     = baud;
    UartHandleDebug.Init.WordLength   = UART_WORDLENGTH_8B;
    UartHandleDebug.Init.StopBits     = UART_STOPBITS_1;
    UartHandleDebug.Init.Parity       = UART_PARITY_NONE;
    UartHandleDebug.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    UartHandleDebug.Init.Mode         = UART_MODE_TX_RX;
    UartHandleDebug.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_DeInit(&UartHandleDebug);
    HAL_UART_Init(&UartHandleDebug);
    //set_logger_output(log_output, ALL_LEVEL); //注册debug实现函数
}

void usart_cellular_initial(uint32_t baud)
{
    HAL_NVIC_DisableIRQ(USART1_IRQn);
    sdkReleaseQueue(&USART_Cellular_Queue);

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    GPIO_InitTypeDef  GPIO_InitStruct;
    /* UART TX GPIO pin configuration  */
    GPIO_InitStruct.Pin       = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FAST;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    UartHandleCellular.Instance          = USART1;
    UartHandleCellular.Init.BaudRate     = baud;
    UartHandleCellular.Init.WordLength   = UART_WORDLENGTH_8B;
    UartHandleCellular.Init.StopBits     = UART_STOPBITS_1;
    UartHandleCellular.Init.Parity       = UART_PARITY_NONE;
    UartHandleCellular.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    UartHandleCellular.Init.Mode         = UART_MODE_TX_RX;
    UartHandleCellular.Init.OverSampling = UART_OVERSAMPLING_16;

    HAL_UART_DeInit(&UartHandleCellular);
    HAL_UART_Init(&UartHandleCellular);

    sdkInitialQueue(&USART_Cellular_Queue, CELLULAR_USART_QUEUE_SIZE); //初始化cellular接受缓冲队列
    //Configure the NVIC for UART
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
    __HAL_UART_ENABLE_IT(&UartHandleCellular, UART_IT_RXNE);
}

void HAL_USART1_Cellular_Handler(UART_HandleTypeDef *huart)
{
    uint8_t c;

    if((__HAL_UART_GET_FLAG(huart, UART_FLAG_RXNE) != RESET)
            && (__HAL_UART_GET_IT_SOURCE(huart, UART_IT_RXNE) != RESET))
    {
        c = (uint16_t)(huart->Instance->DR & (uint16_t)0x00FF);
        sdkInsertQueue(&USART_Cellular_Queue, &c, 1);
    }
}

void Cellular_GPIO_Initial(void)
{
    GPIO_InitTypeDef   GPIO_InitStruct;

    //ESP8266_EN
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Pin = ESP8266_EN_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(ESP8266_EN_GPIO_PORT, &GPIO_InitStruct);
    //ESP8266_RST  PA10
    GPIO_InitStruct.Pin = ESP8266_RST_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(ESP8266_RST_GPIO_PORT, &GPIO_InitStruct);
    //SIM800C PWK_KEY
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitStruct.Pin = SIM800C_PWR_KEY_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    HAL_GPIO_WritePin(SIM800C_PWR_KEY_GPIO_PORT, SIM800C_PWR_KEY_GPIO_PIN, GPIO_PIN_SET);//高电平
    //SIM800C PWK_EN
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitStruct.Pin = SIM800C_PWR_EN_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(SIM800C_PWR_EN_GPIO_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(SIM800C_PWR_EN_GPIO_PORT, SIM800C_PWR_EN_GPIO_PIN, GPIO_PIN_RESET);
}

void Cellular_Power_On(void)
{
    //esp8266 暂停工作
    HAL_GPIO_WritePin(ESP8266_EN_GPIO_PORT, ESP8266_EN_GPIO_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(ESP8266_RST_GPIO_PORT, ESP8266_RST_GPIO_PIN, GPIO_PIN_SET);
    //sim800c 重新开机
    HAL_GPIO_WritePin(SIM800C_PWR_EN_GPIO_PORT, SIM800C_PWR_EN_GPIO_PIN, GPIO_PIN_RESET);
    HAL_Delay(1200);
    HAL_GPIO_WritePin(SIM800C_PWR_EN_GPIO_PORT, SIM800C_PWR_EN_GPIO_PIN, GPIO_PIN_SET);
}

void Cellular_Enter_UpdateMode_Pre(void)
{
    HAL_NVIC_DisableIRQ(USART1_IRQn);
    sdkReleaseQueue(&USART_Cellular_Queue);

    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef  GPIO_InitStruct;
    GPIO_InitStruct.Pin       = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode      = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void Cellular_Enter_UpdateMode(void)
{
    //PWK_KEY 低电平
    HAL_GPIO_WritePin(SIM800C_PWR_KEY_GPIO_PORT, SIM800C_PWR_KEY_GPIO_PIN, GPIO_PIN_SET);

    //PWK_EN
    HAL_GPIO_WritePin(SIM800C_PWR_EN_GPIO_PORT, SIM800C_PWR_EN_GPIO_PIN, GPIO_PIN_RESET);
    HAL_Delay(1000);
    HAL_GPIO_WritePin(SIM800C_PWR_EN_GPIO_PORT, SIM800C_PWR_EN_GPIO_PIN, GPIO_PIN_SET);
}

void HAL_System_Config(void)
{
    Set_System();
#if defined(DEBUG_BUILD)
    usart_debug_initial(115200);
#endif
    HAL_UI_Initial();
    HAL_UI_UserLED_Control(0);
    HAL_RTC_Initial();
    usart_cellular_initial(115200);  //通讯采取115200波特率
    Cellular_GPIO_Initial();
    Cellular_Power_On();
}

system_tick_t millis(void)
{
    return HAL_Tick_Get_Milli_Seconds();
}

unsigned long micros(void)
{
    return HAL_Tick_Get_Micro_Seconds();
}

void delay(uint32_t ms)
{
    unsigned long startTime = millis();
    while( millis() - startTime < ms)
    {
    }
}

void System_Reset(void)
{
    NVIC_SystemReset();
}

