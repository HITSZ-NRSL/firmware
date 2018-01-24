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
#include "intorobot_macros.h"
#include "wiring_ex_bridge.h"
#include "wiring_ex_process.h"

int main() {
    USB_Cable_Initial();
    //open bridge
    DEBUG_D("Bridge Connecting\r\n");
    Bridge.begin();
    DEBUG_D("Bridge Connected\r\n");
    app_setup_and_loop();
    return 0;
}

uint16_t HAL_Core_Get_Subsys_Version(char* buffer, uint16_t len)
{
    String tmp="";
    uint16_t templen;
    Process Proc;

    Proc.begin("openwrt_update_online");
    Proc.addParameter("GETVERSION");
    int res = Proc.run();
    if(res == 0) {
        while (Proc.available()) {
            tmp+=(char)Proc.read();
        }
        if(tmp.length()) {
            templen = MIN(tmp.length(), len-1);
            memset(buffer, 0, len);
            memcpy(buffer, tmp.c_str(), templen);
            return templen;
        }
    }
    return 0;
}

