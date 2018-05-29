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
#include "watchdog_hal.h"
#include "rng_hal.h"
#include "ui_hal.h"
#include "updater_hal.h"
#include "gpio_hal.h"
#include "interrupts_hal.h"
#include "hw_config.h"
#include "rtc_hal.h"
#include "stm32f1xx_it.h"
#include "delay_hal.h"
#include "params_hal.h"
#include "bkpreg_hal.h"

/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/
void HAL_Core_Setup(void);
extern "C" {
uint32_t freeheap();
}
/* Private macro ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Extern variables ----------------------------------------------------------*/

void SysTick_Handler(void)
{
    HAL_IncTick();
    System1MsTick();
    HAL_SysTick_Handler();
}

// 以下为HAL层接口
void HAL_Core_Init(void)
{
}

void HAL_Core_Config(void)
{
    Set_System();
    //设置管脚的默认值
#if !defined(USE_SWD_JTAG) && !defined(USE_SWD)
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_AFIO_REMAP_SWJ_DISABLE();
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
        DEBUG_D("init params fac\r\n");
        HAL_PARAMS_Init_Fac_System_Params();
    } else if(INITPARAM_FLAG_ALL_RESET == HAL_PARAMS_Get_Boot_initparam_flag()) {
        //初始化所有参数
        DEBUG_D("init params all\r\n");
        HAL_PARAMS_Init_All_System_Params();
    }

    if(INITPARAM_FLAG_NORMAL != HAL_PARAMS_Get_Boot_initparam_flag()) {
        HAL_PARAMS_Set_Boot_initparam_flag(INITPARAM_FLAG_NORMAL);
    }
}

void HAL_Core_Setup(void)
{
    HAL_IWDG_Config(DISABLE);
    HAL_Core_Load_Params();
}

void HAL_Core_System_Reset(void)
{
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

//恢复出厂设置 不清除密钥
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
}

void HAL_Core_Enter_Stop_Mode(uint16_t wakeUpPin, uint16_t edgeTriggerMode, long seconds)
{
}

void HAL_Core_Execute_Stop_Mode(void)
{
}

void HAL_Core_Enter_Standby_Mode(void)
{
}

void HAL_Core_Execute_Standby_Mode(void)
{
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
