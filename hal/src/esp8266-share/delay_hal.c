/**
 ******************************************************************************
 * @file    delay_hal.c
 * @author  Matthew McGowan
 * @version V1.0.0
 * @date    25-Sept-2014
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

#include "core_hal.h"
#include "delay_hal.h"
#include "hw_config.h"
#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "cont.h"
#include "core_hal_esp8266.h"

/*******************************************************************************
* Function Name  : Delay
* Description    : Inserts a delay time.
* Input          : nTime: specifies the delay time length, in milliseconds.
* Output         : None
* Return         : None
*******************************************************************************/
void HAL_Delay_Milliseconds(uint32_t millis)
{
    volatile system_tick_t start_millis, current_millis, elapsed_millis;

    start_millis = HAL_Tick_Get_Milli_Seconds();
    while (1)
    {
        current_millis = HAL_Tick_Get_Milli_Seconds();
        elapsed_millis = current_millis - start_millis;
        //Check for wrapping
        if (elapsed_millis < 0)
        {
            elapsed_millis =  0xFFFFFFFF - start_millis + current_millis;
        }

        if (elapsed_millis >= (long)millis)
        {
            break;
        }
        optimistic_yield(100);
    }
}

/*******************************************************************************
 * Function Name  : Delay_Microsecond
 * Description    : Inserts a delay time in microseconds using 32-bit DWT->CYCCNT
 * Input          : uSec: specifies the delay time length, in microseconds.
 * Output         : None
 * Return         : None
 *******************************************************************************/
void HAL_Delay_Microseconds(uint32_t micros)
{
    os_delay_us(micros);
}

