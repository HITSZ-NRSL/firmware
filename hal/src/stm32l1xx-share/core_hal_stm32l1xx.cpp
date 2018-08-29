/**
 ******************************************************************************
  Copyright (c) 2013-2014 IntoRobot Team.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include "core_hal.h"
#include "core_hal_stm32l1xx.h"
#include "watchdog_hal.h"
#include "rng_hal.h"
#include "ui_hal.h"
#include "updater_hal.h"
#include "gpio_hal.h"
#include "interrupts_hal.h"
#include "hw_config.h"
#include "rtc_hal.h"
#include "stm32l1xx_it.h"
#include "params_hal.h"
#include "bkpreg_hal.h"
#include "usb_hal.h"


const static char *TAG = "hal-core";

/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/
void HAL_Core_Setup(void);
extern "C" {
uint32_t freeheap();
}
/* Private macro ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Extern variables ----------------------------------------------------------*/


int main()
{
    HAL_Core_Setup();
    app_setup_and_loop();
    return 0;
}

// 中断调用函数
void SysTick_Handler(void)
{
    HAL_IncTick();
    System1MsTick();
    /* Handle short and generic tasks for the device HAL on 1ms ticks */
    HAL_SysTick_Handler();
    HAL_UI_SysTick_Handler();
    //HAL_System_Interrupt_Trigger(SysInterrupt_SysTick, NULL);
}

void HAL_Core_Init(void)
{
}

void HAL_Core_Config(void)
{
    Set_System();
#ifdef DFU_BUILD_ENABLE
    NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x6000);
#endif
    //设置管脚的默认值
#if !defined(USE_SWD_JTAG) && !defined(USE_SWD)
    for (pin_t pin = FIRST_DIGITAL_PIN; pin <= FIRST_DIGITAL_PIN + TOTAL_DIGITAL_PINS; pin++) {
        //HAL_Pin_Mode(pin, INPUT);
    }

    for (pin_t pin = FIRST_ANALOG_PIN; pin <= FIRST_ANALOG_PIN + TOTAL_ANALOG_PINS; pin++) {
        //HAL_Pin_Mode(pin, INPUT);
    }
#endif
    HAL_RTC_Initial();
    HAL_RNG_Initial();
    HAL_IWDG_Initial();
    HAL_UI_Initial();
}

static void HAL_Core_Load_Params(void)
{
    //加载bootloader和系统参数
    HAL_PARAMS_Load_System_Params();
    HAL_PARAMS_Load_Boot_Params();

    if(INITPARAM_FLAG_FACTORY_RESET == HAL_PARAMS_Get_Boot_initparam_flag()) {
        //初始化参数 保留密钥
        MOLMC_LOGD(TAG, "init params fac");
        HAL_PARAMS_Init_Fac_System_Params();
    } else if(INITPARAM_FLAG_ALL_RESET == HAL_PARAMS_Get_Boot_initparam_flag()) {
        //初始化所有参数
        MOLMC_LOGD(TAG, "init params all");
        HAL_PARAMS_Init_All_System_Params();
    }

    if(INITPARAM_FLAG_NORMAL != HAL_PARAMS_Get_Boot_initparam_flag()) {
        HAL_PARAMS_Set_Boot_initparam_flag(INITPARAM_FLAG_NORMAL);
        HAL_PARAMS_Save_Params();
    }
}

void HAL_Core_Setup(void)
{
    HAL_IWDG_Config(DISABLE);
    HAL_Core_Load_Params();
}

void HAL_Core_System_Reset(void)
{
    HAL_Core_Write_Backup_Register(BKP_DR_03, 0x7DEA);
    NVIC_SystemReset();
}

void HAL_Core_Enter_DFU_Mode(bool persist)
{
    // true  - DFU mode persist if firmware upgrade is not completed
    // false - Briefly enter DFU bootloader mode (works with latest bootloader only )
    //         Subsequent reset or power off-on will execute normal firmware
    if (persist) {
        HAL_PARAMS_Set_Boot_boot_flag(BOOT_FLAG_USB_DFU);
        HAL_PARAMS_Save_Params();
    } else {
        HAL_Core_Write_Backup_Register(BKP_DR_01, 0x7DEA);
    }
    HAL_Core_System_Reset();
}

void HAL_Core_Enter_Com_Mode(void)
{
    HAL_PARAMS_Set_Boot_boot_flag(BOOT_FLAG_SERIAL_COM);
    HAL_PARAMS_Save_Params();
    HAL_Core_System_Reset();
}

// 恢复出厂设置 不清除密钥
void HAL_Core_Enter_Factory_Reset_Mode(void)
{
    HAL_PARAMS_Set_Boot_boot_flag(BOOT_FLAG_FACTORY_RESET);
    HAL_PARAMS_Save_Params();
    HAL_Core_System_Reset();
}

void HAL_Core_Enter_Ota_Update_Mode(void)
{
    HAL_PARAMS_Set_Boot_boot_flag(BOOT_FLAG_OTA_UPDATE);
    HAL_PARAMS_Save_Params();
    HAL_Core_System_Reset();
}

void HAL_Core_Enter_Bootloader(bool persist)
{
    HAL_Core_Write_Backup_Register(BKP_DR_01,0x7DEA);
    HAL_Core_System_Reset();
}

//mcu唤醒后时钟等设置
static void McuRecoverSystem(void)
{
    Set_System();
#ifdef configHAL_USB_CDC_ENABLE
    USB_USART_Initial(115200);
#endif

#ifndef configNO_RGB_UI && configNO_SETUPBUTTON_UI
    HAL_UI_Initial(); //初始化三色灯
#endif
}

//进入低功耗模式前处理mcu相关接口
static void McuLowPowerSystemProcess(void)
{
    /* __HAL_RCC_USB_FORCE_RESET();//USB 如果打开了 必须运行来降低功耗 USB_USART_Initial(0)会运行此功能*/
    __HAL_RCC_LSI_DISABLE();

    //禁用比较器
    __COMP_CLK_DISABLE();

    // 允许/禁用 调试端口 少800uA
    HAL_DBGMCU_DisableDBGStopMode();

    // Disable the Power Voltage Detector
    HAL_PWR_DisablePVD( );

    SET_BIT( PWR->CR, PWR_CR_CWUF );

    // Enable Ultra low power mode
    HAL_PWREx_EnableUltraLowPower( );

    // Enable the fast wake up from Ultra low power mode
    HAL_PWREx_EnableFastWakeUp( );
}

//低功耗模式下引脚设置
static void McuLowPowerSetPin(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    /* Enable GPIOs clock */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    /* Configure all GPIO port pins in Analog Input mode (floating input trigger OFF) */
    //无用IO需设为模拟输入
    GPIO_InitStructure.Pin = GPIO_PIN_All;
    GPIO_InitStructure.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStructure.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);
    HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);
    HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);
    HAL_GPIO_Init(GPIOD, &GPIO_InitStructure);
    HAL_GPIO_Init(GPIOH, &GPIO_InitStructure);

    //处理sx1278 spi 接口 IO 设为输入上拉 降低1278功耗
    GPIO_InitStructure.Pin = GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5;
    GPIO_InitStructure.Mode = INPUT;
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);
    HAL_Pin_Mode(SX1278_NSS,INPUT_PULLUP);
}

void HAL_Core_Enter_Stop_Mode(uint16_t wakeUpPin, uint16_t edgeTriggerMode, long seconds)
{
    MOLMC_LOGD(TAG, "mcu into stop mode");
    McuLowPowerSetPin();
    if(wakeUpPin != 0xFF){ //运行外部中断唤醒
        HAL_Interrupts_Attach(wakeUpPin,NULL,NULL,edgeTriggerMode,NULL);
    }

#ifdef configHAL_USB_CDC_ENABLE
    USB_USART_Initial(0);
#endif

    McuLowPowerSystemProcess();
    HAL_Core_Execute_Stop_Mode();
    McuRecoverSystem();
}

void HAL_Core_Execute_Stop_Mode(void)
{
    // Enter Stop Mode
    HAL_PWR_EnterSTOPMode( PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI );
}

void HAL_Core_Enter_Standby_Mode(uint32_t seconds, void* reserved)
{
    MOLMC_LOGD(TAG, "mcu into standby mode");
    McuLowPowerSetPin();

#ifdef configHAL_USB_CDC_ENABLE
    USB_USART_Initial(0);
#endif
    McuLowPowerSystemProcess();
    #if 0
    /* Enable WKUP pin */
#if PLATFORM_ID == PLATFORM_L6
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    GPIO_InitStructure.Pin = GPIO_PIN_0;
    GPIO_InitStructure.Mode = INPUT;
    GPIO_InitStructure.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);

    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);
#endif
    #endif

    HAL_Core_Execute_Standby_Mode();
}

void HAL_Core_Execute_Standby_Mode(void)
{
    HAL_PWR_EnterSTANDBYMode();
}

uint16_t HAL_Core_Get_Subsys_Version(char* buffer, uint16_t len)
{
    return 0;
}

void HAL_Core_System_Yield(void)
{
}

uint32_t HAL_Core_Runtime_Info(runtime_info_t* info, void* reserved)
{
    info->freeheap = freeheap();
    return 0;
}

void HAL_Core_Enter_Config(void)
{
}

void HAL_Core_Exit_Config(void)
{
}

