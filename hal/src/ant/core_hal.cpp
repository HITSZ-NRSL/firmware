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
//#include <stdatomic.h>
#include <string.h>
#include "core_hal.h"
#include "watchdog_hal.h"
#include "rng_hal.h"
#include "ui_hal.h"
#include "ota_flash_hal.h"
#include "gpio_hal.h"
#include "interrupts_hal.h"
#include "hw_config.h"
#include "syshealth_hal.h"
#include "rtc_hal.h"
#include "stm32l1xx_it.h"
#include "service_debug.h"
#include "params_hal.h"
#include "bkpreg_hal.h"

/* Private typedef ----------------------------------------------------------*/

/* Private define -----------------------------------------------------------*/
void HAL_Core_Setup(void);

/* Private macro ------------------------------------------------------------*/

/* Private variables --------------------------------------------------------*/

/* Extern variables ----------------------------------------------------------*/
/**
 * Updated by HAL_1Ms_Tick()
 */
extern volatile uint32_t TimingDelay;

volatile bool systick_hook_enabled = false;

void HAL_SysTick_Hook(void) __attribute__((weak));

void HAL_SysTick_Hook(void)
{

}

void HAL_Hook_Main() __attribute__((weak));

void HAL_Hook_Main()
{
    // nada
}

int main() {
    HAL_Core_Setup();
    app_setup_and_loop();
    return 0;
}

void HAL_1Ms_Tick()
{
    if (TimingDelay != 0x00)
    {
        __sync_sub_and_fetch(&TimingDelay, 1);
    }
}

void HAL_Core_Init(void)
{

}

void HAL_Core_Config(void)
{
    DECLARE_SYS_HEALTH(ENTERED_Config);
    Set_System();

#ifdef DFU_BUILD_ENABLE
    //Currently this is done through WICED library API so commented.
    NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x6000);
#endif

    //Wiring pins default to inputs
#if !defined(USE_SWD_JTAG) && !defined(USE_SWD)
    for (pin_t pin=0; pin<=7; pin++)
//        HAL_Pin_Mode(pin, INPUT);
    for (pin_t pin=30; pin<=37; pin++)
//        HAL_Pin_Mode(pin, INPUT);
#endif
    HAL_RTC_Initial();
    HAL_RNG_Initial();
    HAL_IWDG_Initial();
    HAL_UI_Initial();

    HAL_UI_RGB_Color(RGB_COLOR_CYAN);
}

void HAL_Core_Load_params(void)
{
    // load params
    HAL_PARAMS_Load_System_Params();
    HAL_PARAMS_Load_Boot_Params();
    // check if need init params
    if(INITPARAM_FLAG_FACTORY_RESET == HAL_PARAMS_Get_Boot_initparam_flag()) //初始化参数 保留密钥
    {
        DEBUG("init params fac");
        HAL_PARAMS_Init_Fac_System_Params();
    }
    else if(INITPARAM_FLAG_ALL_RESET == HAL_PARAMS_Get_Boot_initparam_flag()) //初始化所有参数
    {
        DEBUG("init params all");
        HAL_PARAMS_Init_All_System_Params();
    }

    if(INITPARAM_FLAG_NORMAL != HAL_PARAMS_Get_Boot_initparam_flag()) //保存参数
    {
        HAL_PARAMS_Set_Boot_initparam_flag(INITPARAM_FLAG_NORMAL);
    }

    //保存子系统程序版本号
    char subsys_ver1[32] = {0}, subsys_ver2[32] = {0};
    HAL_Core_Get_Subsys_Version(subsys_ver1, sizeof(subsys_ver1));
    HAL_PARAMS_Get_System_subsys_ver(subsys_ver2, sizeof(subsys_ver2));
    if(strcmp(subsys_ver1, subsys_ver2))
    {
        HAL_PARAMS_Set_System_subsys_ver(subsys_ver1);
    }
}


void HAL_Core_Setup(void)
{
    HAL_IWDG_Config(DISABLE);
    HAL_Core_Load_params();
    HAL_Bootloader_Update_If_Needed();
    //HAL_Bootloader_Lock(true);
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
    if (persist)
    {
        HAL_PARAMS_Set_Boot_boot_flag(BOOT_FLAG_USB_DFU);
        HAL_PARAMS_Save_Params();
    }
    else
    {
        HAL_Core_Write_Backup_Register(BKP_DR_01, 0x7DEA);
    }
    HAL_Core_System_Reset();
}

void HAL_Core_Enter_Config_Mode(void)
{
    /*
    HAL_PARAMS_Set_System_config_flag(!HAL_PARAMS_Get_System_config_flag());
    HAL_PARAMS_Save_Params();
    HAL_Core_System_Reset();
    */
}

void HAL_Core_Enter_Firmware_Recovery_Mode(void)
{
    HAL_PARAMS_Set_Boot_boot_flag(BOOT_FLAG_DEFAULT_RESTORE);
    HAL_PARAMS_Save_Params();
    HAL_Core_System_Reset();
}

void HAL_Core_Enter_Com_Mode(void)
{
    HAL_PARAMS_Set_Boot_boot_flag(BOOT_FLAG_SERIAL_COM);
    HAL_PARAMS_Save_Params();
    HAL_Core_System_Reset();
}
/**
 * 恢复出厂设置 不清除密钥
 */

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

/**
 * 恢复出厂设置 清除密钥
 */
void HAL_Core_Enter_Factory_All_Reset_Mode(void)
{
    HAL_PARAMS_Set_Boot_boot_flag(BOOT_FLAG_ALL_RESET);
    HAL_PARAMS_Save_Params();
    HAL_Core_System_Reset();
}

void HAL_Core_Enter_Safe_Mode(void* reserved)
{
}

void HAL_Core_Enter_Bootloader(bool persist)
{
}

uint16_t HAL_Core_Get_Subsys_Version(char* buffer, uint16_t len)
{
    return 0;
}

void HAL_Core_Set_System_Loop_Handler(void (*handler)(void))
{

}

void HAL_Core_System_Loop(void)
{

}

void HAL_Core_System_Yield(void)
{

}

uint32_t HAL_Core_Runtime_Info(runtime_info_t* info, void* reserved)
{
    return 0;
}


/*******************************************************************************
 * Function Name  : SysTick_Handler
 * Description    : This function handles SysTick Handler.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void SysTick_Handler(void)
{
    HAL_IncTick();
    System1MsTick();
    /* Handle short and generic tasks for the device HAL on 1ms ticks */
    HAL_1Ms_Tick();
    HAL_SysTick_Handler();
    HAL_UI_SysTick_Handler();
    //HAL_System_Interrupt_Trigger(SysInterrupt_SysTick, NULL);
}

