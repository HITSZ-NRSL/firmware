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
#include "esp8266-hal-gpio.h"
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
    EESP8266_Pin_Info *PIN_MAP = HAL_Pin_Map();

    if (!is_valid_pin(pin))
        return PF_NONE;
    if (pinFunction==PF_ADC && PIN_MAP[pin].adc_channel!=ADC_CHANNEL_NONE)
        return PF_ADC;
    if (pinFunction==PF_TIMER && PIN_MAP[pin].timer_peripheral!=NONE)
        return PF_TIMER;
    return PF_DIO;
}

/*
 * @brief Set the mode of the pin to OUTPUT, INPUT, INPUT_PULLUP,
 * or INPUT_PULLDOWN
 */
void HAL_Pin_Mode(pin_t pin, PinMode setMode)
{
    uint8_t mode = ESP8266_OUTPUT;
    EESP8266_Pin_Info *PIN_MAP = HAL_Pin_Map();
    pin_t gpio_pin = PIN_MAP[pin].gpio_pin;

    switch (setMode)
    {
        case OUTPUT:
            PIN_MAP[pin].pin_mode = OUTPUT;
            mode = ESP8266_OUTPUT;
            break;

        case INPUT:
            PIN_MAP[pin].pin_mode = INPUT;
            mode = ESP8266_INPUT;
            break;

        case INPUT_PULLUP:
            PIN_MAP[pin].pin_mode = INPUT_PULLUP;
            mode = ESP8266_INPUT_PULLUP;
            break;

        case INPUT_PULLDOWN:
            PIN_MAP[pin].pin_mode = INPUT_PULLDOWN;
            mode = ESP8266_INPUT_PULLDOWN_16;
            break;

        //Used internally begin esp8266没有相关操作，模拟stm32编写
        case AF_OUTPUT_PUSHPULL:  //Used internally for Alternate Function Output PushPull(TIM, UART, SPI etc)
            PIN_MAP[pin].pin_mode = AF_OUTPUT_PUSHPULL;
            mode = ESP8266_OUTPUT;
            break;

        case AF_OUTPUT_DRAIN:   //Used internally for Alternate Function Output Drain(I2C etc)
            PIN_MAP[pin].pin_mode = AF_OUTPUT_DRAIN;
            mode = ESP8266_OUTPUT_OPEN_DRAIN;
            break;

        case AN_INPUT:        //Used internally for ADC Input
            PIN_MAP[pin].pin_mode = AN_INPUT;
            mode = ESP8266_ANALOG;
            break;

        case AN_OUTPUT:       //Used internally for DAC Output
            PIN_MAP[pin].pin_mode = AN_OUTPUT;
            break;
        //Used internally end

        default:
            break;
    }
    __pinMode(gpio_pin, mode);
}

/*
 * @brief Saves a pin mode to be recalled later.
 */
void HAL_GPIO_Save_Pin_Mode(uint16_t pin)
{
    EESP8266_Pin_Info *PIN_MAP = HAL_Pin_Map();

    uint32_t uprop = (uint32_t)PIN_MAP[pin].user_property;
    uprop = (uprop & 0xFFFF) | (((uint32_t)PIN_MAP[pin].pin_mode & 0xFF) << 16) | (0xAA << 24);
    PIN_MAP[pin].user_property = (int32_t)uprop;
}

/*
 * @brief Recalls a saved pin mode.
 */
PinMode HAL_GPIO_Recall_Pin_Mode(uint16_t pin)
{
    EESP8266_Pin_Info *PIN_MAP = HAL_Pin_Map();
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
    EESP8266_Pin_Info *PIN_MAP = HAL_Pin_Map();
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
    __digitalWrite(PIN_MAP[pin].gpio_pin, value);
}

/*
 * @brief Reads the value of a GPIO pin. Should return either 1 (HIGH) or 0 (LOW).
 */
int32_t HAL_GPIO_Read(uint16_t pin)
{
    EESP8266_Pin_Info *PIN_MAP = HAL_Pin_Map();
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
    return __digitalRead(PIN_MAP[pin].gpio_pin);
}

#define WAIT_FOR_PIN_STATE(value) \
    while (HAL_pinReadFast(pin) != (value)) { \
        if (SYSTEM_TICK_COUNTER - start_cycle_count > timeout_cycles) { \
            return 0; \
        } \
        optimistic_yield(5000); \
    }

/*
 * @brief   blocking call to measure a high or low pulse
 * @returns uint32_t pulse width in microseconds up to 3 seconds,
 *          returns 0 on 3 second timeout error, or invalid pin.
 * max timeout is 27 seconds at 160MHz clock and 54 seconds at 80MHz clock
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
    EESP8266_Pin_Info *PIN_MAP = HAL_Pin_Map();
    __digitalWrite(PIN_MAP[pin].gpio_pin, 1);
}

void HAL_pinResetFast(pin_t pin)
{
    EESP8266_Pin_Info *PIN_MAP = HAL_Pin_Map();
    __digitalWrite(PIN_MAP[pin].gpio_pin, 0);
}

int32_t HAL_pinReadFast(pin_t pin)
{
    EESP8266_Pin_Info *PIN_MAP = HAL_Pin_Map();
    return __digitalRead(PIN_MAP[pin].gpio_pin);
}

