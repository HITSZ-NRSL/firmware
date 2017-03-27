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

#include <stdlib.h>
#include "ota_flash_hal.h"
#include "wiring_ex_process.h"
#include "core_hal.h"

uint8_t down_progress=0;

static bool bootloader_requires_update(void)
{
    return false;
}

static bool bootloader_update(void)
{
    return true;
}

bool HAL_Bootloader_Update_If_Needed(void)
{
    bool updated = false;

    if (bootloader_requires_update()) {
        updated = bootloader_update();
    }
    return updated;
}

down_status_t HAL_OTA_Download_App(const char *host, const char *param, const char * md5)
{
    String url="";
    Process Proc;

    url+=host;
    url+=param;
    Proc.begin("stm32_update_online");
    Proc.addParameter("DOWN");
    Proc.addParameter(url);
    Proc.addParameter(md5);
    int res = Proc.run();
    {
        return DOWNSTATUS_SUCCESS;
    }
    return DOWNSTATUS_FAIL;
}

down_status_t HAL_OTA_Get_App_Download_Status(void)
{
    return DOWNSTATUS_SUCCESS;
}

void HAL_OTA_Update_App(uint32_t size)
{

}

down_status_t HAL_OTA_Download_Subsys(const char *host, const char *param)
{
    String url="";
    Process Proc;

    url+=host;
    url+=param;
    Proc.begin("openwrt_update_online");
    Proc.addParameter("DOWN");
    Proc.addParameter("0.0.0.0");
    Proc.addParameter(url);

    Proc.runAsynchronously();
    if(Proc.running())
    {
        down_progress = 0;
        return DOWNSTATUS_DOWNING;
    }
    else
    {
        if(Proc.exitValue() == 0)
        return DOWNSTATUS_SUCCESS;
    }
    return DOWNSTATUS_FAIL;
}

down_status_t HAL_OTA_Get_Subsys_Download_Status(void)
{
    char c;
    String process;
    Process Proc;

    if(Proc.running())
    {
        process="";
        if(Proc.available()&&('<'==(char)Proc.read()))
        {
            while (Proc.available())//获取进度
            {
                c=(char)Proc.read();
                if('>'==c)
                {break;}
                process+=c;
            }
            if(process.length())
            {
                down_progress = atoi(process.c_str());
            }
            return DOWNSTATUS_DOWNING;
        }
    }
    else
    {
        if(Proc.exitValue() == 0)
        {
            return DOWNSTATUS_SUCCESS;
        }
    }
    return DOWNSTATUS_FAIL;
}

void HAL_OTA_Upadate_Subsys(uint32_t defAppSize, uint32_t bootSize, bool flag)
{
    Process Proc;

    Proc.begin("openwrt_update_online");
    Proc.addParameter("UPDATE");
    Proc.run();
}

uint8_t HAL_OTA_Get_Download_Progress()
{
    return down_progress;
}

