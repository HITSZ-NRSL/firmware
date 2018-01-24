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

#include "hw_config.h"
#include "delay_hal.h"
#include "timer_hal.h"
//#include "watchdog_hal.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*******************************************************************************
* Function Name  : Delay
* Description    : Inserts a delay time.
* Input          : nTime: specifies the delay time length, in milliseconds.
* Output         : None
* Return         : None
*******************************************************************************/
void HAL_Delay_Milliseconds(uint32_t millis)
{
    vTaskDelay(millis / portTICK_PERIOD_MS);
}

/**
 * @brief  delay time in microseconds
 * @param  uSec: specifies the delay time length, in milliseconds.
 * @retval None
 */
void HAL_Delay_Microseconds(uint32_t micros)
{
    uint32_t m = HAL_Timer_Get_Micro_Seconds();
    if(micros){
        uint32_t e = (m + micros) % ((0xFFFFFFFF / CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ) + 1);
        if(m > e){ //overflow
            while(HAL_Timer_Get_Micro_Seconds() > e){
                NOP();
            }
        }
        while(HAL_Timer_Get_Micro_Seconds() < e){
            NOP();
        }
    }
}

