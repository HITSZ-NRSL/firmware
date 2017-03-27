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
#include <stdio.h>

#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp32-hal-timer.h"
#include "esp32-hal-wifi.h"

#include "hw_config.h"
#include "core_hal.h"
#include "rng_hal.h"
#include "ui_hal.h"
#include "ota_flash_hal.h"
#include "gpio_hal.h"
#include "interrupts_hal.h"
#include "syshealth_hal.h"
#include "intorobot_macros.h"
#include "rtc_hal.h"
#include "service_debug.h"
#include "delay_hal.h"
#include "timer_hal.h"
#include "params_hal.h"
#include "bkpreg_hal.h"
#include "flash_map.h"
#include "memory_hal.h"
#include "driver/timer.h"
#include "esp_attr.h"
#include "eeprom_hal.h"
#include "subsys_version.h"

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/portmacro.h"
#include "nvs_flash.h"
}


void init() __attribute__((weak));
void init() {}

void initVariant() __attribute__((weak));
void initVariant() {}

void HAL_Core_Config(void);
void HAL_Core_Setup(void);

static void application_task_start(void *pvParameters)
{
    HAL_Core_Config();
    HAL_Core_Setup();
    app_setup_and_loop();
}

//处理三色灯和模式处理
void SysTick_Handler(void);
static void ui_task_start(void *pvParameters)
{
    while(1) {
        HAL_Delay_Milliseconds(1);
        SysTick_Handler();
    }
}
extern "C" const char intorobot_subsys_version[32] __attribute__((section(".subsys.version"))) = SUBSYS_VERSION ;
extern "C" void app_main()
{
    nvs_flash_init();
    init();
    initVariant();
    printf("\n%08x\n", intorobot_subsys_version);
    xTaskCreatePinnedToCore(application_task_start, "app_thread", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(ui_task_start, "ui_thread", 4096, NULL, 1, NULL, 1);
}

void HAL_Core_Init(void)
{

}

void HAL_Core_Config(void)
{
    /*
    for (pin_t pin=FIRST_DIGITAL_PIN; pin<=FIRST_DIGITAL_PIN + TOTAL_DIGITAL_PINS; pin++) {
        HAL_Pin_Mode(pin, INPUT);
    }

    for (pin_t pin=FIRST_ANALOG_PIN; pin<=FIRST_ANALOG_PIN + TOTAL_ANALOG_PINS; pin++) {
        HAL_Pin_Mode(pin, INPUT);
    }
    */

    HAL_RTC_Initial();
    HAL_RNG_Initial();

    HAL_IWDG_Initial();
    HAL_UI_Initial();
    HAL_EEPROM_Init();

    HAL_UI_RGB_Color(RGB_COLOR_CYAN);
    esp32_setMode(WIFI_MODE_STA);    // wifi初始化
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
    if(INITPARAM_FLAG_NORMAL != HAL_PARAMS_Get_Boot_initparam_flag()) //初始化参数 保留密钥
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
    HAL_SubSystem_Update_If_Needed();
    HAL_Bootloader_Update_If_Needed();
}

void HAL_Core_System_Reset(void)
{
    HAL_Core_Write_Backup_Register(BKP_DR_03, 0x7DEA);
    esp_restart();
}

void HAL_Core_Enter_DFU_Mode(bool persist)
{
}

void HAL_Core_Enter_Config_Mode(void)
{
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
    char data[32];
    uint16_t templen;

    if (buffer!=NULL && len>0) {
        spi_flash_read(SUBSYS_VERSION_ADDR, (uint32_t*) data, sizeof(data));
        if(!memcmp(data, "VERSION:", 8))
        {
            templen = MIN(strlen(&data[8]), len-1);
            memset(buffer, 0, len);
            memcpy(buffer, &data[8], templen);
            return templen;
        }
    }
    return 0;
}

void HAL_Core_Set_System_Loop_Handler(void (*handler)(void))
{
}

void SysTick_Handler(void)
{
    HAL_SysTick_Handler();
    HAL_UI_SysTick_Handler();
}

void HAL_Core_System_Yield(void)
{
}

uint32_t HAL_Core_Runtime_Info(runtime_info_t* info, void* reserved)
{
    return 0;
}
