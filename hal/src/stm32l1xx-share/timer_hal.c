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
#include "stm32l1xx_it.h"

static TIM_HandleTypeDef TimHandle;
static TimerCallback_t _timerCallback = NULL;
uint32_t _elapsedTickStart = 0;

void HAL_Timer_Start(uint32_t timeout)
{
    _elapsedTickStart = DWT->CYCCNT;
    HAL_Timer_Stop();
    __HAL_RCC_TIM4_CLK_ENABLE(); //必须要放在HAL_TIM_Base_Init之前

    timeout *= 10;
    if(timeout < 3) {  //如果timeout等于0 需要等计数溢出才会触发中断
        timeout = 3;
    } else if (timeout > 0x10000) {
        timeout = 0x10000;
    }

    TimHandle.Instance = TIM4;
    TimHandle.Init.Period = timeout - 1;
    TimHandle.Init.Prescaler = (uint32_t) ((SystemCoreClock / 10000) - 1);;
    TimHandle.Init.ClockDivision = 0;
    TimHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
    HAL_TIM_Base_Init(&TimHandle);

    HAL_NVIC_SetPriority(TIM4_IRQn, TIM_IRQ_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(TIM4_IRQn);

    HAL_TIM_Base_Start_IT(&TimHandle);
    __HAL_TIM_CLEAR_IT(&TimHandle, TIM_IT_UPDATE); //防止定时器开启立即进入定时器中断
}

void HAL_Timer_Stop(void)
{
    __HAL_RCC_TIM4_CLK_DISABLE();
    HAL_NVIC_DisableIRQ(TIM4_IRQn);
}

uint32_t HAL_Timer_Get_ElapsedTime(void)
{
    uint32_t elapsedTick = 0;
    uint32_t _elapsedTickCurrent = DWT->CYCCNT;

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

void TIM4_IRQHandler(void)
{
    if(__HAL_TIM_GET_FLAG(&TimHandle, TIM_FLAG_UPDATE) != RESET) {
        if(__HAL_TIM_GET_IT_SOURCE(&TimHandle, TIM_IT_UPDATE) != RESET) {
            __HAL_TIM_CLEAR_IT(&TimHandle, TIM_IT_UPDATE);
            if(_timerCallback) {
                HAL_Timer_Stop();
                _timerCallback();
            }
        }
    }
}

