/**
 ******************************************************************************
 * @file    interrupts_hal.c
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
#include "interrupts_hal.h"
#include "gpio_hal.h"
#include "pinmap_impl.h"
#include "rom/gpio.h"
#include "soc/gpio_struct.h"
#include "soc/gpio_reg.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_intr.h"
#include "esp32-hal-gpio.h"


static HAL_InterruptHandler exti_handle;
static void *exti_data = NULL;

void HAL_CallbackExtiHandle(void)
{
    HAL_InterruptHandler isr_handle = exti_handle;
    if (isr_handle)
    {
        isr_handle(exti_data);
    }
}

void HAL_Interrupts_Attach(uint16_t pin, HAL_InterruptHandler handler, void* data, InterruptMode mode, HAL_InterruptExtraConfiguration* config)
{
    EESP32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    pin_t gpio_pin = PIN_MAP[pin].gpio_pin;
    if (gpio_pin > 39) {
        return;
    }

    uint8_t interruptMode;
    switch(mode)
    {
        case CHANGE:
            interruptMode = ESP32_CHANGE;
            break;

        case RISING:
            interruptMode = ESP32_RISING;
            break;

        case FALLING:
            interruptMode = ESP32_FALLING;
            break;

        default:
            break;
    }

    exti_handle = handler;
    exti_data = data;
    __attachInterrupt(gpio_pin,HAL_CallbackExtiHandle,interruptMode);
}

void HAL_Interrupts_Detach(uint16_t pin)
{
    EESP32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    pin_t gpio_pin = PIN_MAP[pin].gpio_pin;
    if (gpio_pin > 39) {
        return;
    }
    __detachInterrupt(gpio_pin);
}

void HAL_Interrupts_Enable_All(void)
{
}

void HAL_Interrupts_Disable_All(void)
{
}

void HAL_Interrupts_Suspend(void)
{
}

void HAL_Interrupts_Restore(void)
{
}

/*******************************************************************************
 * Function Name  : HAL_EXTI_Handler (Declared as weak in stm32_it.cpp)
 * Description    : This function is called by any of the interrupt handlers. It
                                                 essentially fetches the user function pointer from the array
                                                 and calls it.
 * Input          : EXTI_Line (Supported: 0, 1, 3, 4, 5, 6, 7, 13, 14 and 15)
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void HAL_EXTI_Handler(uint8_t EXTI_Line)
{
    //fetch the user function pointer from the array
}


int HAL_disable_irq()
{
    return 0;
}

void HAL_enable_irq(int is)
{
}

uint8_t HAL_IsISR()
{
	return 0;
}
