/**
 ******************************************************************************
 * @file    timers_hal.c
 * @authors robin
 * @version V1.0.0
 * @date    19-06-2017
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
#include "timer_hal.h"

static os_timer_t system_timer;
static TimerCallback_t _timerCallback = NULL;
uint32_t _elapsedTickStart = 0;

static void _system_timer_handler(void* arg)
{
    if(_timerCallback) {
        HAL_Timer_Stop();
        _timerCallback();
    }
}

void HAL_Timer_Start(uint32_t timeout)
{
    _elapsedTickStart = system_get_time();

    if(timeout < 1) {
        timeout = 1;
    }

    HAL_Timer_Stop();
    os_timer_setfn(&system_timer, (os_timer_func_t*)&_system_timer_handler, 0);
    os_timer_arm(&system_timer, timeout, 1);
}

void HAL_Timer_Stop(void)
{
    os_timer_disarm(&system_timer);
}

uint32_t HAL_Timer_Get_ElapsedTime(void)
{
    uint32_t elapsedTick = 0;
    uint32_t _elapsedTickCurrent = system_get_time();

    if (_elapsedTickCurrent < _elapsedTickStart) {
        elapsedTick =  UINT_MAX - _elapsedTickStart + _elapsedTickCurrent;
    } else {
        elapsedTick = _elapsedTickCurrent - _elapsedTickStart;
    }

    return elapsedTick / 1000;
}

void HAL_Timer_Set_Callback(TimerCallback_t callback)
{
    _timerCallback = callback;
}

