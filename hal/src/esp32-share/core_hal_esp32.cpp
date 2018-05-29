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
#include "core_hal_esp32.h"
#include "rng_hal.h"
#include "ui_hal.h"
#include "updater_hal.h"
#include "gpio_hal.h"
#include "interrupts_hal.h"
#include "intorobot_macros.h"
#include "rtc_hal.h"
#include "delay_hal.h"
#include "timer_hal.h"
#include "params_hal.h"
#include "bkpreg_hal.h"
#include "flash_map.h"
#include "driver/timer.h"
#include "esp_attr.h"
#include "eeprom_hal.h"

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

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

extern "C" void app_main()
{
    esp_log_level_set("*", (esp_log_level_t)CONFIG_LOG_DEFAULT_LEVEL);
    nvs_flash_init();
    init();
    initVariant();
    xTaskCreatePinnedToCore(application_task_start, "app_thread", 4096, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
    xTaskCreatePinnedToCore(ui_task_start, "ui_thread", 4096, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
}

void SysTick_Handler(void)
{
    HAL_SysTick_Handler();
    HAL_UI_SysTick_Handler();
}

void HAL_Core_Init(void)
{
}

void HAL_Core_Config(void)
{
    //设置管脚的默认值
    for (pin_t pin = FIRST_DIGITAL_PIN; pin <= FIRST_DIGITAL_PIN + TOTAL_DIGITAL_PINS; pin++) {
        //HAL_Pin_Mode(pin, INPUT);
    }
    for (pin_t pin = FIRST_ANALOG_PIN; pin <= FIRST_ANALOG_PIN + TOTAL_ANALOG_PINS; pin++) {
        //HAL_Pin_Mode(pin, INPUT);
    }

    HAL_RTC_Initial();
    HAL_RNG_Initial();
    HAL_IWDG_Initial();
    HAL_UI_Initial();

    esp32_setMode(WIFI_MODE_STA);    // wifi初始化
}

void HAL_Core_Load_Params(void)
{
    //加载bootloader和系统参数
    HAL_PARAMS_Load_System_Params();
    HAL_PARAMS_Load_Boot_Params();

    if(INITPARAM_FLAG_FACTORY_RESET == HAL_PARAMS_Get_Boot_initparam_flag()) {
        //初始化参数 保留密钥
        DEBUG("init params fac\r\n");
        HAL_PARAMS_Init_Fac_System_Params();
    } else if(INITPARAM_FLAG_ALL_RESET == HAL_PARAMS_Get_Boot_initparam_flag()) {
        //初始化所有参数
        DEBUG("init params all\r\n");
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
    esp_restart();
}

void HAL_Core_Enter_DFU_Mode(bool persist)
{
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
    info->freeheap = esp_get_free_heap_size();
    return 0;
}

void HAL_Core_Enter_Config(void)
{
}

void HAL_Core_Exit_Config(void)
{
}
