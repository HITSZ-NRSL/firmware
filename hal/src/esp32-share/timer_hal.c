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

#include "timer_hal.h"
#include "hw_config.h"
#include "esp_timer.h"

static esp_timer_handle_t _timer = NULL;
static TimerCallback_t _timerCallback = NULL;
uint32_t _elapsedTickStart = 0;

static uint32_t IRAM_ATTR _get_systemTick(void)
{
    uint32_t ccount;
    __asm__ __volatile__ ( "rsr     %0, ccount" : "=a" (ccount) );
    return ccount;
}

static void _system_timer_handler(void)
{
    if(_timerCallback) {
        HAL_Timer_Stop();
        _timerCallback();
    }
}

void HAL_Timer_Start(uint32_t timeout)
{
    _elapsedTickStart = _get_systemTick();

    if(timeout < 1) {
        timeout = 1;
    }

    esp_timer_create_args_t _timerConfig;
    _timerConfig.arg = (void*)0;
    _timerConfig.callback = _system_timer_handler;
    _timerConfig.dispatch_method = ESP_TIMER_TASK;
    _timerConfig.name = "Ticker";

    if (_timer) {
        esp_timer_stop(_timer);
        esp_timer_delete(_timer);
    }
    esp_timer_create(&_timerConfig, &_timer);
    esp_timer_start_once(_timer, timeout * 1000);
}

void HAL_Timer_Stop(void)
{
    if (_timer) {
        esp_timer_stop(_timer);
        esp_timer_delete(_timer);
        _timer = NULL;
    }
}

uint32_t HAL_Timer_Get_ElapsedTime(void)
{
    uint32_t elapsedTick = 0;
    uint32_t _elapsedTickCurrent = _get_systemTick();

    if (_elapsedTickCurrent < _elapsedTickStart) {
        elapsedTick =  UINT_MAX - _elapsedTickStart + _elapsedTickCurrent;
    } else {
        elapsedTick = _elapsedTickCurrent - _elapsedTickStart;
    }

    return elapsedTick / (SYSTEM_US_TICKS * 1000);
}

void HAL_Timer_Set_Callback(TimerCallback_t callback)
{
    _timerCallback = callback;
}

