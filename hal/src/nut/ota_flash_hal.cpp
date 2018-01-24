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
#include "flash_storage_impl.h"
#include "params_hal.h"
#include "ui_hal.h"

#define OTA_CHUNK_SIZE          0x1000

/*从flash起始地址复制至目标地址*/
bool copy_raw(const uint32_t src_addr, const uint32_t dst_addr, const uint32_t size)
{
    // require regions to be aligned
    if ((src_addr & 0xfff) != 0 ||
        (dst_addr & 0xfff) != 0){
        return false;
    }

    const uint32_t buffer_size = SPI_FLASH_SEC_SIZE;
    uint8_t buffer[buffer_size];
    uint32_t left = ((size+buffer_size-1) & ~(buffer_size-1));
    uint32_t saddr = src_addr;
    uint32_t daddr = dst_addr;
    InternalFlashStore flashStore;

    while (left){
        if (flashStore.eraseSector(daddr/buffer_size)){
            return false;
        }
        if (flashStore.read(saddr, (uint32_t*)buffer, buffer_size)){
            return false;
        }
        if (flashStore.write(daddr, (uint32_t*)buffer, buffer_size)){
            return false;
        }
        saddr += buffer_size;
        daddr += buffer_size;
        left  -= buffer_size;
    }
    return true;
}

static bool bootloader_requires_update(void)
{
    char subsys_ver[32] = {0}, temp[16] = {0};
    char *ptr = NULL;
    uint32_t boot_ver_current = 0, boot_ver_new = 0;

    if(HAL_Core_Get_Subsys_Version(subsys_ver, sizeof(subsys_ver))) {
        boot_ver_current = HAL_PARAMS_Get_Boot_boot_version();
        DEBUG_D("subsys: %s\r\n", subsys_ver);
        if(boot_ver_current) {
            memset(temp, 0, sizeof(temp));
            ptr = strrchr(subsys_ver, '.');
            boot_ver_new = atoi(ptr+1);
            DEBUG_D("boot_current=%d  boot_new=%d \r\n", boot_ver_current, boot_ver_new);
            if(boot_ver_current < boot_ver_new) {
                return true;
            }
        }
    }
    return false;
}

static bool bootloader_update(void)
{
    int count=3;
    uint32_t boot_size = HAL_PARAMS_Get_Boot_boot_size();

    if(boot_size)
    {
        while(count--) {
            if(copy_raw(CACHE_BOOT_ADDR, BOOT_ADDR, boot_size)) {
                HAL_PARAMS_Set_Boot_boot_size(0);
                HAL_PARAMS_Save_Params();
                return true;
            }
        }
    }
    return false;
}

/*注意 该函数不要随意更改*/
bool HAL_Bootloader_Update_If_Needed(void)
{
    bool updated = false;
    /*注意 bootloader升级过程不允许保存参数，因为bootloader暂放在系统参数存储区*/
    if (bootloader_requires_update()) {
        HAL_UI_RGB_Color(RGB_COLOR_YELLOW);
        updated = bootloader_update();
    }
    return updated;
}

bool HAL_SubSystem_Update_If_Needed(void)
{
    if (HAL_PARAMS_Get_System_subsys_flag()) {
        HAL_UI_RGB_Color(RGB_COLOR_YELLOW);
        HAL_OTA_Upadate_Subsys(0, 0, false);
    }
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
    return esp8266_downDefaultApp(host, param);
}

down_status_t HAL_OTA_Get_Subsys_Download_Status(void)
{
    return esp8266_getDownDefaultAppStatus();
}

void HAL_OTA_Upadate_Subsys(uint32_t defAppSize, uint32_t bootSize, bool flag)
{
    int count=3;

    HAL_PARAMS_Set_System_subsys_flag(1);
    HAL_PARAMS_Save_Params();
    uint32_t def_app_size = HAL_PARAMS_Get_Boot_def_app_size();

    if(def_app_size) {
        while(count--) {
            if(copy_raw(CACHE_DEFAULT_APP_ADDR, DEFAULT_APP_ADDR, def_app_size)) {
                break;
            }
        }
    }
    HAL_PARAMS_Set_System_subsys_flag(0);
    HAL_PARAMS_Save_Params();
}

uint8_t HAL_OTA_Get_Download_Progress()
{
    return esp8266_getDownloadProgress();
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
    return HAL_UPDATE_ERROR;
}

