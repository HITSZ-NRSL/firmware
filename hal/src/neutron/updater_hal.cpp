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
#include "updater_hal.h"
#include "core_hal.h"
#include "delay_hal.h"
#include "parser.h"
#include "flash_map.h"
#include "params_hal.h"
#include "flash_storage_impl.h"
#include "molmc_log.h"

static THandlerFunction_Progress _update_handle = NULL;

bool HAL_Update(const char *host, const char *uri, const char * md5, updater_mode_t mode)
{
    bool flag = false;
    deal_status_t status;

    deal_status_t result = esp8266MDM.downOtaFile(host, uri, "");
    switch(result) {
        case DEALSTATUS_SUCCESS:
            flag = true;
            break;
        case DEALSTATUS_DOING:
            do {
                status = esp8266MDM.getDownOtafileStatus();
                if(DEALSTATUS_SUCCESS == status) {
                    HAL_Set_Update_Flag(0);
                    flag = true;
                    break;
                } else if(DEALSTATUS_DOING == status) {
                    if(NULL != _update_handle) {
                        _update_handle(NULL, 0, esp8266MDM.getDownFileProgress(),100);
                    }
                } else if(DEALSTATUS_FAIL == status) {
                    break;
                }
            } while(1);
            break;
        default:
            break;
    }
    return flag;
}

void HAL_Set_Update_Handle(THandlerFunction_Progress fn)
{
    _update_handle = fn;
}

void HAL_Set_Update_Flag(uint32_t size)
{
    HAL_PARAMS_Set_Boot_boot_flag(BOOT_FLAG_OTA_UPDATE);
    HAL_PARAMS_Save_Params();
}

uint32_t HAL_Update_StartAddress()
{
    return 0;
}

uint32_t HAL_Update_FlashLength()
{
    return 0;
}

int HAL_Update_Flash(const uint8_t *pBuffer, uint32_t address, uint32_t length, void* reserved)
{
    return 0;
}
