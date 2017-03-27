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

#include "hw_config.h"
#include "ota_flash_hal.h"
#include "esp8266_downfile.h"
#include "core_hal.h"
#include "flash_map.h"
#include "memory_hal.h"
#include "flash_utils.h"
#include "params_hal.h"


#define OTA_CHUNK_SIZE          0x1000


bool HAL_Bootloader_Update_If_Needed(void)
{
    return false;
}

bool HAL_SubSystem_Update_If_Needed(void)
{
    return true;
}

down_status_t HAL_OTA_Download_App(const char *host, const char *param, const char * md5)
{
    return esp8266_downOnlineApp(host, param, "");
}

down_status_t HAL_OTA_Get_App_Download_Status(void)
{
    return esp8266_getDownOnlineAppStatus();
}

void HAL_OTA_Update_App(uint32_t size)
{
    HAL_Core_Enter_Ota_Update_Mode();
}

down_status_t HAL_OTA_Download_Subsys(const char *host, const char *param)
{
    return DOWNSTATUS_SUCCESS;
}

down_status_t HAL_OTA_Get_Subsys_Download_Status(void)
{
    return DOWNSTATUS_SUCCESS;
}

void HAL_OTA_Upadate_Subsys(uint32_t defAppSize, uint32_t bootSize, bool flag)
{
}

uint8_t HAL_OTA_Get_Download_Progress()
{
    return 0;
}

bool HAL_OTA_CheckValidAddressRange(uint32_t startAddress, uint32_t length)
{
    return true;
}

uint32_t HAL_OTA_FlashAddress()
{
    return CACHE_ONLINE_APP_ADDR;
}

uint32_t HAL_OTA_FlashLength()
{
    return CACHE_ONLINE_APP_SEC_NUM * SPI_FLASH_SEC_SIZE;
}

uint16_t HAL_OTA_ChunkSize()
{
    return OTA_CHUNK_SIZE;
}

bool HAL_FLASH_Begin(uint32_t address, uint32_t length, void* reserved)
{
    return true;
}

int HAL_FLASH_Update(const uint8_t *pBuffer, uint32_t address, uint32_t length, void* reserved)
{
    return 0;
}

hal_update_complete_t HAL_FLASH_End(void)
{
    return 0;
}

