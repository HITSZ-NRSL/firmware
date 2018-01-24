/**
 ******************************************************************************
  Copyright (c) 2013-2014 IntoRobot Team.  All right reserved.

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

/* Includes -----------------------------------------------------------------*/
#include "hw_config.h"
#include "gpio_hal.h"
#include "pinmap_impl.h"
#include <stddef.h>
#include "dac_hal.h"

/* Private typedef ----------------------------------------------------------*/

/* Private define -----------------------------------------------------------*/

/* Private macro ------------------------------------------------------------*/

/* Private variables --------------------------------------------------------*/

/* Extern variables ---------------------------------------------------------*/

/* Private function prototypes ----------------------------------------------*/

inline bool is_valid_pin(pin_t pin) __attribute__((always_inline));
inline bool is_valid_pin(pin_t pin)
{
    return pin<TOTAL_PINS;
}

PinMode HAL_Get_Pin_Mode(pin_t pin)
{
    return (!is_valid_pin(pin)) ? PIN_MODE_NONE : HAL_Pin_Map()[pin].pin_mode;
}

PinFunction HAL_Validate_Pin_Function(pin_t pin, PinFunction pinFunction)
{
    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();

    if (!is_valid_pin(pin))
        return PF_NONE;
    if (pinFunction==PF_ADC && PIN_MAP[pin].adc_channel!=ADC_CHANNEL_NONE)
        return PF_ADC;
    if (pinFunction==PF_DAC && PIN_MAP[pin].dac_channel!=DAC_CHANNEL_NONE)
        return PF_DAC;
    if (pinFunction==PF_TIMER && PIN_MAP[pin].timer_peripheral!=NULL)
        return PF_TIMER;
    return PF_DIO;
}

/*
 * @brief Set the mode of the pin to OUTPUT, INPUT, INPUT_PULLUP,
 * or INPUT_PULLDOWN
 */
void HAL_Pin_Mode(pin_t pin, PinMode setMode)
{
    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();

    GPIO_TypeDef *gpio_port = PIN_MAP[pin].gpio_peripheral;
    pin_t gpio_pin = PIN_MAP[pin].gpio_pin;

    // Initialize GPIO_InitStructure to fix system wake up from pin function.
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    if (gpio_port == GPIOA)
    {
        __HAL_RCC_GPIOA_CLK_ENABLE();
    }
    else if (gpio_port == GPIOB)
    {
        __HAL_RCC_GPIOB_CLK_ENABLE();
    }
    else if (gpio_port == GPIOC)
    {
        __HAL_RCC_GPIOC_CLK_ENABLE();
    }
    else if (gpio_port == GPIOD)
    {
        __HAL_RCC_GPIOD_CLK_ENABLE();
    }

    GPIO_InitStructure.Pin = gpio_pin;

    switch (setMode)
    {
        case OUTPUT:
            GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
            GPIO_InitStructure.Pull = GPIO_PULLUP;
            GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
            PIN_MAP[pin].pin_mode = OUTPUT;
            break;

        case INPUT:
            GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
            GPIO_InitStructure.Pull = GPIO_NOPULL;
            GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
            PIN_MAP[pin].pin_mode = INPUT;
            break;

        case INPUT_PULLUP:
            GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
            GPIO_InitStructure.Pull = GPIO_PULLUP;
            GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
            PIN_MAP[pin].pin_mode = INPUT_PULLUP;
            break;

        case INPUT_PULLDOWN:
            GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
            GPIO_InitStructure.Pull = GPIO_PULLDOWN;
            GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
            PIN_MAP[pin].pin_mode = INPUT_PULLDOWN;
            break;

        case AF_OUTPUT_PUSHPULL:  //Used internally for Alternate Function Output PushPull(TIM, UART, SPI etc)
            GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
            GPIO_InitStructure.Pull = GPIO_NOPULL;
            GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
            PIN_MAP[pin].pin_mode = AF_OUTPUT_PUSHPULL;
            break;

        case AF_OUTPUT_DRAIN:   //Used internally for Alternate Function Output Drain(I2C etc)
            GPIO_InitStructure.Mode = GPIO_MODE_AF_OD;
            GPIO_InitStructure.Pull = GPIO_PULLUP;
            GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
            PIN_MAP[pin].pin_mode = AF_OUTPUT_DRAIN;
            break;

        case AN_INPUT:        //Used internally for ADC Input
            GPIO_InitStructure.Mode = GPIO_MODE_ANALOG;
            GPIO_InitStructure.Pull = GPIO_NOPULL;
            GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
            PIN_MAP[pin].pin_mode = AN_INPUT;
            break;

        default:
            break;
    }

    HAL_GPIO_Init(gpio_port, &GPIO_InitStructure);
}

/*
 * @brief Saves a pin mode to be recalled later.
 */
void HAL_GPIO_Save_Pin_Mode(uint16_t pin)
{
    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    uint32_t uprop = (uint32_t)PIN_MAP[pin].user_property;
    uprop = (uprop & 0xFFFF) | (((uint32_t)PIN_MAP[pin].pin_mode & 0xFF) << 16) | (0xAA << 24);
    PIN_MAP[pin].user_property = (int32_t)uprop;
}

/*
 * @brief Recalls a saved pin mode.
 */
PinMode HAL_GPIO_Recall_Pin_Mode(uint16_t pin)
{
    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    uint32_t uprop = (uint32_t)PIN_MAP[pin].user_property;
    if ((uprop & 0xFF000000) != 0xAA000000)
        return PIN_MODE_NONE;
    PinMode pm = (PinMode)((uprop & 0x00FF0000) >> 16);

    // Safety check
    switch(pm)
    {
        case INPUT:
        case OUTPUT:
        case INPUT_PULLUP:
        case INPUT_PULLDOWN:
        case AF_OUTPUT_PUSHPULL:
        case AF_OUTPUT_DRAIN:
        case AN_INPUT:
        case AN_OUTPUT:
        break;

        default:
        pm = PIN_MODE_NONE;
        break;
    }
    return pm;
}

/*
 * @brief Sets a GPIO pin to HIGH or LOW.
 */
void HAL_GPIO_Write(uint16_t pin, uint8_t value)
{
    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    //If the pin is used by analogWrite, we need to change the mode
    if(PIN_MAP[pin].pin_mode == AF_OUTPUT_PUSHPULL)
    {
        HAL_Pin_Mode(pin, OUTPUT);
    }
    else if (PIN_MAP[pin].pin_mode == AN_OUTPUT)
    {
        if (HAL_DAC_Is_Enabled(pin))
            HAL_DAC_Enable(pin, 0);
        HAL_Pin_Mode(pin, OUTPUT);
    }

    if(value == 0)
    {
        HAL_GPIO_WritePin(PIN_MAP[pin].gpio_peripheral, PIN_MAP[pin].gpio_pin, GPIO_PIN_RESET);
    }
    else
    {
        HAL_GPIO_WritePin(PIN_MAP[pin].gpio_peripheral, PIN_MAP[pin].gpio_pin, GPIO_PIN_SET);
    }
}

/*
 * @brief Reads the value of a GPIO pin. Should return either 1 (HIGH) or 0 (LOW).
 */
int32_t HAL_GPIO_Read(uint16_t pin)
{
    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    if(PIN_MAP[pin].pin_mode == AN_INPUT)
    {
        PinMode pm = HAL_GPIO_Recall_Pin_Mode(pin);
        if(pm == PIN_MODE_NONE)
        {
            return 0;
        }
        else
        {
            // Restore the PinMode after calling analogRead() on same pin earlier
            HAL_Pin_Mode(pin, pm);
        }
    }
    else if (PIN_MAP[pin].pin_mode == AN_OUTPUT)
    {
        PinMode pm = HAL_GPIO_Recall_Pin_Mode(pin);
        if(pm == PIN_MODE_NONE)
        {
            return 0;
        }
        else
        {
            // Disable DAC
            if (HAL_DAC_Is_Enabled(pin))
                HAL_DAC_Enable(pin, 0);
            // Restore pin mode
            HAL_Pin_Mode(pin, pm);
        }
    }

    if(PIN_MAP[pin].pin_mode == OUTPUT)
    {
        return HAL_GPIO_ReadPin(PIN_MAP[pin].gpio_peripheral, PIN_MAP[pin].gpio_pin);
    }

    return HAL_GPIO_ReadPin(PIN_MAP[pin].gpio_peripheral, PIN_MAP[pin].gpio_pin);
}

#define WAIT_FOR_PIN_STATE(value) \
    while (HAL_pinReadFast(pin) != (value)) { \
        if (SYSTEM_TICK_COUNTER - start_cycle_count > timeout_cycles) { \
            return 0; \
        } \
    }

/*
 * @brief   blocking call to measure a high or low pulse
 * @returns uint32_t pulse width in microseconds up to 3 seconds,
 *          returns 0 on 3 second timeout error, or invalid pin.
 */
uint32_t HAL_Pulse_In(pin_t pin, uint16_t value, uint32_t timeout)
{
    if (timeout > 3000000UL) {
        timeout = 3000000UL;
    }
    const uint32_t timeout_cycles = microsecondsToClockCycles(timeout);
    const uint32_t start_cycle_count = SYSTEM_TICK_COUNTER;
    WAIT_FOR_PIN_STATE(!value);
    WAIT_FOR_PIN_STATE(value);
    const uint32_t pulse_start_cycle_count = SYSTEM_TICK_COUNTER;
    WAIT_FOR_PIN_STATE(!value);
    return clockCyclesToMicroseconds(SYSTEM_TICK_COUNTER - pulse_start_cycle_count);
}

void HAL_pinModeFast(pin_t pin, PinMode mode)
{
    HAL_Pin_Mode(pin, mode);
}

void HAL_pinSetFast(pin_t pin)
{
    STM32_Pin_Info *PIN_MAP = HAL_Pin_Map();
    PIN_MAP[pin].gpio_peripheral->BSRR = PIN_MAP[pin].gpio_pin;
}

void HAL_pinResetFast(pin_t pin)
{
    STM32_Pin_Info *PIN_MAP = HAL_Pin_Map();
    PIN_MAP[pin].gpio_peripheral->BSRR = (uint32_t)PIN_MAP[pin].gpio_pin << 16U;
}

int32_t HAL_pinReadFast(pin_t pin)
{
    STM32_Pin_Info *PIN_MAP = HAL_Pin_Map();
    return ((PIN_MAP[pin].gpio_peripheral->IDR & PIN_MAP[pin].gpio_pin) == 0 ? 0 : 1);
}

