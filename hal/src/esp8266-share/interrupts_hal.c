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
#include "hw_config.h"
#include "interrupts_hal.h"
#include "pinmap_impl.h"

//Interrupt Modes
#define ESP8266_INTERRUPT_MODE_RISING    0x01
#define ESP8266_INTERRUPT_MODE_FALLING   0x02
#define ESP8266_INTERRUPT_MODE_CHANGE    0x03
#define ESP8266_INTERRUPT_MODE_ONLOW     0x04
#define ESP8266_INTERRUPT_MODE_ONHIGH    0x05
#define ESP8266_INTERRUPT_MODE_ONLOW_WE  0x0C
#define ESP8266_INTERRUPT_MODE_ONHIGH_WE 0x0D

// Create a structure for user ISR function pointers
typedef struct exti_channel {
    uint8_t mode;
    HAL_InterruptHandler fn;
    void* data;
} exti_channel;

//Array to hold user ISR function pointers
static exti_channel exti_channels[16];
static uint32_t interrupt_reg = 0;
static uint32_t savedPS = 0;

void ICACHE_RAM_ATTR HAL_EXTI_Handler(void);

void HAL_Interrupts_Initial(void)
{
    ETS_GPIO_INTR_ATTACH(HAL_EXTI_Handler, &interrupt_reg);
    ETS_GPIO_INTR_ENABLE();
}

void HAL_Interrupts_Attach(uint16_t pin, HAL_InterruptHandler handler, void* data, InterruptMode mode, HAL_InterruptExtraConfiguration* config)
{
    EESP8266_Pin_Info* PIN_MAP = HAL_Pin_Map();

    pin_t gpio_pin = PIN_MAP[pin].gpio_pin;
    uint8_t GPIO_PinSource = PIN_MAP[pin].gpio_pin_source;

    //gpio16 不支持gpio外部终端
    if(gpio_pin == 16)
    {
        return;
    }

    // Register the handler for the user function name
    exti_channels[GPIO_PinSource].fn = handler;
    exti_channels[GPIO_PinSource].data = data;

    switch (mode)
    {
        //case LOW:
        //There is no LOW mode in STM32, so using falling edge as default
        //exti_channels[GPIO_PinSource].mode = ESP8266_INTERRUPT_MODE_ONLOW;
        //break;
        case CHANGE:
            //generate interrupt on rising or falling edge
            exti_channels[GPIO_PinSource].mode = ESP8266_INTERRUPT_MODE_CHANGE;
            break;
        case RISING:
            //generate interrupt on rising edge
            exti_channels[GPIO_PinSource].mode = ESP8266_INTERRUPT_MODE_RISING;
            break;
        case FALLING:
            //generate interrupt on falling edge
            exti_channels[GPIO_PinSource].mode = ESP8266_INTERRUPT_MODE_FALLING;
            break;
    }

    interrupt_reg |= (1 << gpio_pin);
    GPC(gpio_pin) &= ~(0xF << GPCI);//INT mode disabled
    GPIEC = (1 << gpio_pin); //Clear Interrupt for this pin
    GPC(gpio_pin) |= ((exti_channels[GPIO_PinSource].mode & 0xF) << GPCI);//INT mode "mode"
}


void HAL_Interrupts_Detach(uint16_t pin)
{
    EESP8266_Pin_Info* PIN_MAP = HAL_Pin_Map();

    pin_t gpio_pin = PIN_MAP[pin].gpio_pin;
    uint8_t GPIO_PinSource = PIN_MAP[pin].gpio_pin_source;

    //gpio16 不支持gpio外部终端
    if(gpio_pin == 16)
    {
        return;
    }

    GPC(gpio_pin) &= ~(0xF << GPCI);//INT mode disabled
    GPIEC = (1 << gpio_pin); //Clear Interrupt for this pin
    interrupt_reg &= ~(1 << gpio_pin);

    //unregister the user's handler
    exti_channels[GPIO_PinSource].mode = 0;
    exti_channels[GPIO_PinSource].fn = NULL;
    exti_channels[GPIO_PinSource].data = NULL;
}

void HAL_Interrupts_Enable_All(void)
{
    xt_rsil(0);
}

void HAL_Interrupts_Disable_All(void)
{
    xt_rsil(15);
}

void HAL_Interrupts_Suspend(void)
{
    savedPS = xt_rsil(15); // stop other interrupts
}

void HAL_Interrupts_Restore(void)
{
    xt_wsr_ps(savedPS);
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
void ICACHE_RAM_ATTR HAL_EXTI_Handler(void)
{
    uint32_t status = GPIE;

    GPIEC = status;//clear them interrupts
    uint32_t levels = GPI;
    if(status == 0 || interrupt_reg == 0) return;
    ETS_GPIO_INTR_DISABLE();
    int i = 0;
    uint32_t changedbits = status & interrupt_reg;
    while(changedbits){
        while(!(changedbits & (1 << i))) i++;
        changedbits &= ~(1 << i);
        if (exti_channels[i].fn &&
                (exti_channels[i].mode == CHANGE ||
                 (exti_channels[i].mode & 1) == !!(levels & (1 << i)))) {
            // to make ISR compatible to Arduino AVR model where interrupts are disabled
            // we disable them before we call the client ISR
            uint32_t savedPS = xt_rsil(15); // stop other interrupts
            //fetch the user function pointer from the array
            void* data = exti_channels[i].data;
            HAL_InterruptHandler userISR_Handle = exti_channels[i].fn;
            if (userISR_Handle)
            {
                userISR_Handle(data);
            }
            xt_wsr_ps(savedPS);
        }
    }
    ETS_GPIO_INTR_ENABLE();
}

int HAL_disable_irq()
{
    return xt_rsil(15); // disable all interrupts
}

void HAL_enable_irq(int is)
{
    xt_rsil(0); // enable all interrupts
}

uint8_t HAL_IsISR()
{
    return 0;
}

/* For now, we remember only one handler, but in future this may be extended to a
 * dynamically linked list to allow for multiple handlers.
 */
static HAL_InterruptCallback SystemInterruptHandlers[__Last_irq];


inline bool is_valid_irq(hal_irq_t irq)
{
    return irq<__Last_irq;
}

uint8_t HAL_Set_System_Interrupt_Handler(hal_irq_t irq, const HAL_InterruptCallback* callback, HAL_InterruptCallback* previous, void* reserved)
{
    if (!is_valid_irq(irq))
        return false;
    HAL_InterruptCallback cb = SystemInterruptHandlers[irq];
    if (previous)
        *previous = cb;
    if (callback)
        cb = *callback;
    else
        cb.handler = 0;

    return true;
}

uint8_t HAL_Get_System_Interrupt_Handler(hal_irq_t irq, HAL_InterruptCallback* callback, void* reserved)
{
    if (!is_valid_irq(irq))
        return false;

    if (callback) {
        HAL_InterruptCallback cb = SystemInterruptHandlers[irq];
        *callback = cb;
    }

    return true;
}

void HAL_System_Interrupt_Trigger(hal_irq_t irq, void* reserved)
{
    if (is_valid_irq(irq))
    {
        HAL_InterruptCallback cb = SystemInterruptHandlers[irq];
        if (cb.handler)
            cb.handler(cb.data);
    }
}
