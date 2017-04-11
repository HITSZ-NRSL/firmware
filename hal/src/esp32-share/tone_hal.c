/**
 ******************************************************************************
 * @file    tone_hal.c
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
#include "tone_hal.h"
#include "esp32-hal-ledc.h"
#include "esp32-hal-gpio.h"
#include "pinmap_impl.h"

void HAL_Tone_Start(uint8_t pin, uint32_t frequency, uint32_t duration)
{
    EESP32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    pin_t gpio_pin = PIN_MAP[pin].gpio_pin;
    uint8_t channel = PIN_MAP[pin].timer_ch;

    if(channel > 15 || gpio_pin > 27)
    {
        return;
    }
    else
    {
        ledcAttachPin(gpio_pin, channel);
        ledcWriteTone(channel, frequency);
    }
}

void HAL_Tone_Stop(uint8_t pin)
{
}

uint32_t HAL_Tone_Get_Frequency(uint8_t pin)
{
    return 0;
}

bool HAL_Tone_Is_Stopped(uint8_t pin)
{
    return true;
}
