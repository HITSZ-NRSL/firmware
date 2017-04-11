/**
 ******************************************************************************
 * @file    pwm_hal.c
 * @authors Satish Nair, Brett Walach
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
#include "pwm_hal.h"
#include "pinmap_impl.h"
#include "esp32-hal-ledc.h"

#define TIM_NUM   4

typedef struct pwm_state_t {
    uint8_t resolution;
} pwm_state_t;

static pwm_state_t PWM_State[TIM_NUM] = {
    // Initialise all timers to 8-bit resolution
    [0 ... (TIM_NUM - 1)].resolution = 8
};

void HAL_PWM_Write(uint16_t pin, uint8_t value)
{
}

void HAL_PWM_Write_With_Frequency(uint16_t pin, uint8_t value, uint16_t pwm_frequency)
{
}

uint16_t HAL_PWM_Get_Frequency(uint16_t pin)
{
    return 0;
}

uint16_t HAL_PWM_Get_AnalogValue(uint16_t pin)
{
    return 0;
}

void HAL_PWM_Write_Ext(uint16_t pin, uint32_t value)
{
    HAL_PWM_Write_With_Frequency_Ext(pin, value, configTIM_PWM_FREQ);
}


void HAL_PWM_Write_With_Frequency_Ext(uint16_t pin, uint32_t value, uint32_t pwm_frequency)
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
        ledcSetup(channel, pwm_frequency, HAL_PWM_Get_Resolution(pin));
        ledcAttachPin(gpio_pin, channel);
        ledcWrite(channel, value);
    }
}

//channel 0 1 8  9  为Timer0
//channel 2 3 10 11 为Timer1
//channel 4 5 12 13 为Timer2
//channel 6 7 14 15 为Timer3

void HAL_PWM_Set_Resolution(uint16_t pin, uint8_t resolution)
{
    if(resolution < 1 || resolution > 16)
    {
        return;
    }
    else
    {
        EESP32_Pin_Info* PIN_MAP = HAL_Pin_Map();
        pin_t gpio_pin = PIN_MAP[pin].gpio_pin;

        if(PIN_MAP[pin].timer_peripheral == 0)
        {
            PWM_State[0].resolution = resolution;
        }
        else if(PIN_MAP[pin].timer_peripheral == 1)
        {
            PWM_State[1].resolution = resolution;
        }
        else if(PIN_MAP[pin].timer_peripheral == 2)
        {
            PWM_State[2].resolution = resolution;
        }
        else if(PIN_MAP[pin].timer_peripheral == 3)
        {
            PWM_State[3].resolution = resolution;
        }
    }
}


uint8_t HAL_PWM_Get_Resolution(uint16_t pin)
{
    EESP32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    pin_t gpio_pin = PIN_MAP[pin].gpio_pin;

    if(PIN_MAP[pin].timer_peripheral == 0)
    {
        return PWM_State[0].resolution;
    }
    else if(PIN_MAP[pin].timer_peripheral == 1)
    {
        return PWM_State[1].resolution;
    }
    else if(PIN_MAP[pin].timer_peripheral == 2)
    {
        return PWM_State[2].resolution;
    }
    else if(PIN_MAP[pin].timer_peripheral == 3)
    {
        return PWM_State[3].resolution;
    }
}
