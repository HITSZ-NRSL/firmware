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
#ifndef OTA_FLASH_HAL_H
#define	OTA_FLASH_HAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "static_assert.h"

typedef struct key_value {
    const char* key;
    char value[32];
} key_value;


#ifdef	__cplusplus
extern "C" {
#endif

typedef enum {
    DOWNSTATUS_SUCCESS = 0,
    DOWNSTATUS_FAIL    = 1,
    DOWNSTATUS_DOWNING = 2,
}down_status_t;



bool HAL_Bootloader_Update_If_Needed(void);
bool HAL_SubSystem_Update_If_Needed(void);
down_status_t HAL_OTA_Download_App(const char *host, const char *param, const char * md5);
down_status_t HAL_OTA_Get_App_Download_Status(void);
void HAL_OTA_Update_App(uint32_t size);
down_status_t HAL_OTA_Download_Subsys(const char *host, const char *param);
down_status_t HAL_OTA_Get_Subsys_Download_Status(void);
void HAL_OTA_Upadate_Subsys(uint32_t defAppSize, uint32_t bootSize, bool flag);
uint8_t HAL_OTA_Get_Download_Progress();

bool HAL_OTA_CheckValidAddressRange(uint32_t startAddress, uint32_t length);
uint32_t HAL_OTA_FlashAddress();
uint32_t HAL_OTA_FlashLength();
uint16_t HAL_OTA_ChunkSize();
uint32_t HAL_DEF_APP_FlashAddress();
uint32_t HAL_DEF_APP_FlashLength();
uint32_t HAL_BOOT_FlashAddress();
uint32_t HAL_BOOT_FlashLength();

bool HAL_FLASH_Begin(uint32_t address, uint32_t length, void* reserved);
int HAL_FLASH_Update(const uint8_t *pBuffer, uint32_t address, uint32_t length, void* reserved);

typedef enum {
    HAL_UPDATE_ERROR,
    HAL_UPDATE_APPLIED_PENDING_RESTART,
    HAL_UPDATE_APPLIED
} hal_update_complete_t;

hal_update_complete_t HAL_FLASH_End(void);

#ifdef	__cplusplus
}
#endif

#endif	/* OTA_FLASH_HAL_H */
