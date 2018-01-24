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
#include "hw_config.h"
#include "core_hal.h"
#include "flash_map.h"
#include "intorobot_macros.h"


extern "C" const char intorobot_subsys_version_header[8] __attribute__((section(".subsys.version.header"))) = {'V', 'E', 'R', 'S', 'I', 'O', 'N', ':'};
extern "C" const char intorobot_subsys_version[32] __attribute__((section(".subsys.version"))) = stringify(SUBSYS_VERSION_STRING);
uint16_t HAL_Core_Get_Subsys_Version(char* buffer, uint16_t len)
{
    char data[32];
    uint16_t templen;

    printf("\n%08x ", intorobot_subsys_version_header);
    printf("%08x\n", intorobot_subsys_version);
    if (buffer!=NULL && len>0) {
        sFLASH_Init();
        sFLASH_ReadBuffer((uint8_t *)data, SUBSYS_VERSION_ADDR, 32);
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

