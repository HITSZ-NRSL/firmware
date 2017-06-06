/**
******************************************************************************
* @file    servo_hal.c
* @author  Matthew McGowan
* @version V1.0.0
* @date    27-Sept-2014
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

#include "servo_hal.h"
#include "pinmap_impl.h"
#include "esp8266-hal-servo.h"

void HAL_Servo_Attach(uint16_t pin)
{
    EESP8266_Pin_Info* PIN_MAP = HAL_Pin_Map();
    pin_t gpio_pin = PIN_MAP[pin].gpio_pin;
    ESP8266ServoAttach(gpio_pin);
}

void HAL_Servo_Detach(uint16_t pin)
{
    EESP8266_Pin_Info* PIN_MAP = HAL_Pin_Map();
    pin_t gpio_pin = PIN_MAP[pin].gpio_pin;

    ESP8266ServoDetach(gpio_pin);
}

void HAL_Servo_Write_Pulse_Width(uint16_t pin, uint16_t pulseWidth)
{
    EESP8266_Pin_Info* PIN_MAP = HAL_Pin_Map();
    pin_t gpio_pin = PIN_MAP[pin].gpio_pin;

    ESP8266ServoWrite(gpio_pin,pulseWidth);
}

uint16_t HAL_Servo_Read_Pulse_Width(uint16_t pin)
{
    EESP8266_Pin_Info* PIN_MAP = HAL_Pin_Map();
    pin_t gpio_pin = PIN_MAP[pin].gpio_pin;

    return ESP8266ServoRead(gpio_pin);
}

uint16_t HAL_Servo_Read_Frequency(uint16_t pin)
{
    return 0;
}

