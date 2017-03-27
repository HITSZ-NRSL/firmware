/* Copyright (c) 2015-2016 Ivan Grokhotkov. All rights reserved.
 * This file is part of eboot bootloader.
 *
 * Redistribution and use is permitted according to the conditions of the
 * 3-clause BSD license to be found in the LICENSE file.
 */

#include "hw_config.h"
#include "boot_mode.h"
#include "flash.h"
#include "esp8266/esp8266_config.h"
#include "params_hal.h"
#include "bkpreg_hal.h"
#include "boot_debug.h"

//#define BOOTLOADER_VERSION  1
#define BOOTLOADER_VERSION  2       //修改开机三色灯高亮问题


uint8_t FACTORY_RESET_MODE=0;
uint8_t START_APP_MODE=0;
uint8_t OTA_FIRMWARE_MODE=0;


int main()
{
    BOOT_DEBUG("boot start...\r\n");
    Set_System();

    HAL_PARAMS_Load_Boot_Params();
    HAL_PARAMS_Load_System_Params();

    if(BOOTLOADER_VERSION != HAL_PARAMS_Get_Boot_boot_version())
    {
        BOOT_DEBUG("save boot version...\r\n");
        HAL_PARAMS_Set_Boot_boot_version(BOOTLOADER_VERSION);
        HAL_PARAMS_Save_Params();
    }

    switch(HAL_PARAMS_Get_Boot_boot_flag())
    {
        case BOOT_FLAG_NORMAL:          //正常启动
            START_APP_MODE = 1;
            break;
        case BOOT_FLAG_FACTORY_RESET:   //恢复出厂
            FACTORY_RESET_MODE = 1;
            break;
        case BOOT_FLAG_OTA_UPDATE:      //在线升级
            OTA_FIRMWARE_MODE = 1;
            break;
        default:
            break;
    }

    if(FACTORY_RESET_MODE)
    {
        BOOT_DEBUG("factroy reset\r\n");
        Enter_Factory_RESTORE_Mode();
        FACTORY_RESET_MODE = 0;
    }
    else if(OTA_FIRMWARE_MODE)
    {
        BOOT_DEBUG("ota firmware\r\n");
        Enter_OTA_Update_Mode();
        OTA_FIRMWARE_MODE = 0;
    }

    BOOT_DEBUG("start app\r\n");
    start_app();
    return 0;
}

