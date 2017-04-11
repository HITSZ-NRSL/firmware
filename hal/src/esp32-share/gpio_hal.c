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
#include "esp32-hal-gpio.h"

PinMode digitalPinModeSaved = PIN_MODE_NONE;

/* Private function prototypes ----------------------------------------------*/
inline bool is_valid_pin(pin_t pin) __attribute__((always_inline));
inline bool is_valid_pin(pin_t pin)
{
    return (pin < TOTAL_PINS);
}

PinMode HAL_Get_Pin_Mode(pin_t pin)
{
    return (!is_valid_pin(pin)) ? PIN_MODE_NONE : HAL_Pin_Map()[pin].pin_mode;
}

PinFunction HAL_Validate_Pin_Function(pin_t pin, PinFunction pinFunction)
{
    EESP32_Pin_Info* PIN_MAP = HAL_Pin_Map();

    if (!is_valid_pin(pin))
        return PF_NONE;
    if (pinFunction==PF_ADC && PIN_MAP[pin].adc_channel!=ADC_CHANNEL_NONE)
        return PF_ADC;
    if (pinFunction==PF_DAC && PIN_MAP[pin].dac_channel!=DAC_CHANNEL_NONE)
        return PF_DAC;
    if (pinFunction==PF_TIMER && PIN_MAP[pin].timer_peripheral != 0xFF)
        return PF_TIMER;
    return PF_DIO;
}

/*
 * @brief Set the mode of the pin to OUTPUT, INPUT, INPUT_PULLUP,
 * or INPUT_PULLDOWN
 */
void HAL_Pin_Mode(pin_t pin, PinMode setMode)
{
    uint8_t mode;
    EESP32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    pin_t gpio_pin = PIN_MAP[pin].gpio_pin;

    switch (setMode)
    {
        case OUTPUT:
            PIN_MAP[pin].pin_mode = OUTPUT;
            mode = ESP32_OUTPUT;
            break;

        case INPUT:
            PIN_MAP[pin].pin_mode = INPUT;
            mode = ESP32_INPUT;
            break;

        case INPUT_PULLUP:
            PIN_MAP[pin].pin_mode = INPUT_PULLUP;
            mode = ESP32_INPUT_PULLUP;
            break;

        case INPUT_PULLDOWN:
            PIN_MAP[pin].pin_mode = INPUT_PULLDOWN;
            mode = ESP32_INPUT_PULLDOWN;
            break;

        case AF_OUTPUT_DRAIN:   //Used internally for Alternate Function Output Drain(I2C etc)
            PIN_MAP[pin].pin_mode = AF_OUTPUT_DRAIN;
            mode = ESP32_OUTPUT_OPEN_DRAIN;
            break;

        case AN_INPUT:        //Used internally for ADC Input
            PIN_MAP[pin].pin_mode = AN_INPUT;
            mode = ESP32_ANALOG;
            break;

        default:
            break;
    }
    __pinMode(gpio_pin, mode);
}

/*
 * @brief Saves a pin mode to be recalled later.
 */
void HAL_GPIO_Save_Pin_Mode(PinMode mode)
{
    digitalPinModeSaved = mode;
}

/*
 * @brief Recalls a saved pin mode.
 */
PinMode HAL_GPIO_Recall_Pin_Mode()
{
    return digitalPinModeSaved;
}

/*
 * @brief Sets a GPIO pin to HIGH or LOW.
 */
void HAL_GPIO_Write(uint16_t pin, uint8_t value)
{
    EESP32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    pin_t gpio_pin = PIN_MAP[pin].gpio_pin;
    __digitalWrite(gpio_pin,value);
}

/*
 * @brief Reads the value of a GPIO pin. Should return either 1 (HIGH) or 0 (LOW).
 */
int32_t HAL_GPIO_Read(uint16_t pin)
{
    EESP32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    pin_t gpio_pin = PIN_MAP[pin].gpio_pin;
    return __digitalRead(gpio_pin);
}

/*
 * @brief   blocking call to measure a high or low pulse
 * @returns uint32_t pulse width in microseconds up to 3 seconds,
 *          returns 0 on 3 second timeout error, or invalid pin.
 */
uint32_t HAL_Pulse_In(pin_t pin, uint16_t value)
{
    return 0;
}

void HAL_pinSetFast(pin_t pin)
{
    HAL_GPIO_Write(pin, 1);
}

void HAL_pinResetFast(pin_t pin)
{
    HAL_GPIO_Write(pin, 0);
}

int32_t HAL_pinReadFast(pin_t pin)
{
    return HAL_GPIO_Read(pin);
}

