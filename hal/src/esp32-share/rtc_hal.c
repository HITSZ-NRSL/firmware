/**
 ******************************************************************************
 * @file    rtc_hal.c
 * @author  Satish Nair, Brett Walach
 * @version V1.0.0
 * @date    12-Sept-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-2015 IntoRobot Industries, Inc.  All rights reserved.

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
#include "rtc_hal.h"
#include "tick_hal.h"

/* Private typedef -----------------------------------------------------------*/
typedef struct {
    uint32_t magic;
    time_t unix_time_base;
    uint32_t rtc_base;
}rtc_time_t;

/* Private define ------------------------------------------------------------*/
#define  RTC_MAGIC  0x55aaaa55

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static rtc_time_t g_rtc_time;

/* Extern variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/


void HAL_RTC_Initial(void)
{
    memset((void *)&g_rtc_time, 0, sizeof(rtc_time_t));
    g_rtc_time.magic = RTC_MAGIC;
}

time_t HAL_RTC_Get_UnixTime(void)
{
    time_t unix_time;

    if(g_rtc_time.magic = RTC_MAGIC)
    {
        uint32_t elapsed_rtc_time = 0;
        uint32_t rtc_time = HAL_Tick_Get_Milli_Seconds();

        if(rtc_time < g_rtc_time.rtc_base)
        {
            elapsed_rtc_time = 0xFFFFFFFF - g_rtc_time.rtc_base + rtc_time;
            unix_time = g_rtc_time.unix_time_base + elapsed_rtc_time/1000;
            g_rtc_time.unix_time_base = unix_time;
            g_rtc_time.rtc_base = rtc_time;
        }
        else
        {
            elapsed_rtc_time = rtc_time - g_rtc_time.rtc_base;
            unix_time = g_rtc_time.unix_time_base + elapsed_rtc_time/1000;
        }
        return unix_time;
    }
    else
    {
        return 0;
    }
}

void HAL_RTC_Set_UnixTime(time_t value)
{
    if(g_rtc_time.magic = RTC_MAGIC)
    {
        g_rtc_time.unix_time_base = value;
        g_rtc_time.rtc_base = HAL_Tick_Get_Milli_Seconds();
    }
}

void HAL_RTC_Set_Alarm(uint32_t value)
{
}

void HAL_RTC_Set_UnixAlarm(time_t value)
{

}

void HAL_RTC_Cancel_UnixAlarm(void)
{

}

uint8_t HAL_RTC_Time_Is_Valid(void* reserved)
{
    return 0;
}

void HAL_RTC_SetCallbacks(const void (*handle)(), void* reserved)
{
}

