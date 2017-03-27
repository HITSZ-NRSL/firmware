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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "ota_flash_hal.h"
#include "core_hal.h"
#include "delay_hal.h"
#include "parser.h"
#include "flash_map.h"
#include "params_hal.h"
#include "flash_storage_impl.h"
#include "service_debug.h"

static bool bootloader_requires_update(void)
{
    char subsys_ver[32] = {0}, temp[16] = {0};
    char *ptr = NULL;
    uint32_t boot_ver = 0, boot_ver1;

    if(HAL_Core_Get_Subsys_Version(subsys_ver, sizeof(subsys_ver)))
    {
        uint32_t boot_ver = HAL_PARAMS_Get_Boot_boot_version();
        DEBUG_D("boot %s boot_ver = %d \r\n", subsys_ver, boot_ver);
        if(boot_ver)
        {
            memset(temp, 0, sizeof(temp));
            ptr = strrchr(subsys_ver, '.');
            boot_ver1 = atoi(ptr+1);
            DEBUG_D("boot_ver=%d  boot_ver1=%d \r\n", boot_ver, boot_ver1);
            if(boot_ver != boot_ver1)
            {
                return true;
            }
        }
    }
    return false;
}

static bool bootloader_update(void)
{
    const uint32_t buffer_size = 0x400;  //1k 数据块
    const uint32_t size = 0x8000;    //32k 总大小
    uint8_t buffer[buffer_size];

    if( false == esp8266MDM.getBootloader() ){
        return false;
    }

    uint32_t left = ((size+buffer_size-1) & ~(buffer_size-1));
    uint32_t saddr = CACHE_BOOTLOADER_START_ADDR;
    uint32_t daddr = BOOTLOADER_START_ADDR;

    DEBUG_D("bootloader_update \r\n");
    InternalFlashStore flashStore;
    flashStore.eraseSector(BOOTLOADER_START_ADDR);
    flashStore.eraseSector(BOOTLOADER_START_ADDR + 0x4000);

    while (left) {
        if(flashStore.read(saddr, buffer, buffer_size)) {
            return false;
        }

        if(flashStore.write(daddr, buffer, buffer_size)) {
            return false;
        }
        saddr += buffer_size;
        daddr += buffer_size;
        left  -= buffer_size;
    }
    return true;
}

/*注意 该函数不要随意更改*/
bool HAL_Bootloader_Update_If_Needed(void)
{
    bool updated = false;
    /*注意 bootloader升级过程不允许保存参数，因为bootloader暂放在系统参数存储区*/
    if (bootloader_requires_update()) {
        updated = bootloader_update();
    }
    return updated;
}

down_status_t HAL_OTA_Download_App(const char *host, const char *param, const char * md5)
{
    return (down_status_t)esp8266MDM.downOtaFile(host, param, "");
}

down_status_t HAL_OTA_Get_App_Download_Status(void)
{
    return (down_status_t)esp8266MDM.getDownOtafileStatus();
}

void HAL_OTA_Update_App(uint32_t size)
{
    HAL_Core_Enter_Ota_Update_Mode();
}

down_status_t HAL_OTA_Download_Subsys(const char *host, const char *param)
{
    return (down_status_t)esp8266MDM.downNetFile(host, param);
}

down_status_t HAL_OTA_Get_Subsys_Download_Status(void)
{
    return (down_status_t)esp8266MDM.getDownNetfileStatus();
}

void HAL_OTA_Upadate_Subsys(uint32_t defAppSize, uint32_t bootSize, bool flag)
{
    HAL_Bootloader_Update_If_Needed();
    esp8266MDM.updateNet();
    HAL_Delay_Milliseconds(20000);
}

uint8_t HAL_OTA_Get_Download_Progress()
{
    return (uint8_t)esp8266MDM.getDownFileProgress();
}

