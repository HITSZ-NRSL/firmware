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

/* Private typedef ----------------------------------------------------------*/

/* Private define -----------------------------------------------------------*/

/* Private macro ------------------------------------------------------------*/

/* Private variables --------------------------------------------------------*/
PinMode digitalPinModeSaved = PIN_MODE_NONE;
uint8_t esp8266_gpioToFn[16] = {0x34, 0x18, 0x38, 0x14, 0x3C, 0x40, 0x1C, 0x20, 0x24, 0x28, 0x2C, 0x30, 0x04, 0x08, 0x0C, 0x10};

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
    EESP82666_Pin_Info* PIN_MAP = HAL_Pin_Map();

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
    EESP82666_Pin_Info* PIN_MAP = HAL_Pin_Map();

    pin_t gpio_pin = PIN_MAP[pin].gpio_pin;
    if(gpio_pin < 16)
    {
        switch (setMode)
        {
            case OUTPUT:
                GPF(gpio_pin) = GPFFS(GPFFS_GPIO(gpio_pin));//Set mode to GPIO
                GPC(gpio_pin) = (GPC(gpio_pin) & (0xF << GPCI)); //SOURCE(GPIO) | DRIVER(NORMAL) | INT_TYPE(UNCHANGED) | WAKEUP_ENABLE(DISABLED)
                GPES = (1 << gpio_pin); //Enable
                PIN_MAP[pin].pin_mode = OUTPUT;
                break;

            case INPUT:
                GPF(gpio_pin) = GPFFS(GPFFS_GPIO(gpio_pin));//Set mode to GPIO
                GPEC = (1 << gpio_pin); //Disable
                GPC(gpio_pin) = (GPC(gpio_pin) & (0xF << GPCI)) | (1 << GPCD); //SOURCE(GPIO) | DRIVER(OPEN_DRAIN) | INT_TYPE(UNCHANGED) | WAKEUP_ENABLE(DISABLED)
                PIN_MAP[pin].pin_mode = INPUT;
                break;

            case INPUT_PULLUP:
                GPF(gpio_pin) = GPFFS(GPFFS_GPIO(gpio_pin));//Set mode to GPIO
                GPEC = (1 << gpio_pin); //Disable
                GPC(gpio_pin) = (GPC(gpio_pin) & (0xF << GPCI)) | (1 << GPCD); //SOURCE(GPIO) | DRIVER(OPEN_DRAIN) | INT_TYPE(UNCHANGED) | WAKEUP_ENABLE(DISABLED)
                GPF(gpio_pin) |= (1 << GPFPU);  // Enable  Pullup
                PIN_MAP[pin].pin_mode = INPUT_PULLUP;
                break;

            default:
                break;
        }
    }
    else if(gpio_pin == 16)
    {
        switch (setMode)
        {
            case OUTPUT:
                GPF16 = GP16FFS(GPFFS_GPIO(gpio_pin));//Set mode to GPIO
                GPC16 = 0;
                GP16E |= 1;
                PIN_MAP[pin].pin_mode = OUTPUT;
                break;

            case INPUT:
                GPF16 = GP16FFS(GPFFS_GPIO(gpio_pin));//Set mode to GPIO
                GPC16 = 0;
                GP16E &= ~1;
                PIN_MAP[pin].pin_mode = INPUT;
                break;

            case INPUT_PULLDOWN:
                GPF16 = GP16FFS(GPFFS_GPIO(gpio_pin));//Set mode to GPIO
                GPC16 = 0;
                GPF16 |= (1 << GP16FPD);//Enable Pulldown
                GP16E &= ~1;
                PIN_MAP[pin].pin_mode = INPUT_PULLDOWN;
                break;

            default:
                break;
        }
    }
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
    EESP82666_Pin_Info* PIN_MAP = HAL_Pin_Map();

    pin_t gpio_pin = PIN_MAP[pin].gpio_pin;
    if(gpio_pin < 16){
        if(value) GPOS = (1 << gpio_pin);
        else GPOC = (1 << gpio_pin);
    } else if(gpio_pin == 16){
        if(value) GP16O |= 1;
        else GP16O &= ~1;
    }
}

/*
 * @brief Reads the value of a GPIO pin. Should return either 1 (HIGH) or 0 (LOW).
 */
int32_t HAL_GPIO_Read(uint16_t pin)
{
    EESP82666_Pin_Info* PIN_MAP = HAL_Pin_Map();

    pin_t gpio_pin = PIN_MAP[pin].gpio_pin;

    if(gpio_pin < 16){
        return GPIP(gpio_pin);
    } else if(gpio_pin == 16){
        return GP16I & 0x01;
    }
    return 0;
}

/*
 * @brief   blocking call to measure a high or low pulse
 * @returns uint32_t pulse width in microseconds up to 3 seconds,
 *          returns 0 on 3 second timeout error, or invalid pin.
 */
uint32_t HAL_Pulse_In(pin_t pin, uint16_t value)
{
    EESP82666_Pin_Info* SOLO_PIN_MAP = HAL_Pin_Map();
#define pinReadFast(_pin) ( SOLO_PIN_MAP[_pin].gpio_pin < 16 ? GPIP(SOLO_PIN_MAP[_pin].gpio_pin) : (GP16I&0x01))

    // FIXME: SYTME_TICK_COUNTER change to system_get_time which return the micro seconds
    volatile uint32_t timeoutStart = system_get_time(); // total 3 seconds for entire function!

    /* If already on the value we want to measure, wait for the next one.
     * Time out after 3 seconds so we don't block the background tasks
     */
    while (pinReadFast(pin) == value) {
        if (sytem_get_time() - timeoutStart > 3000000UL) {
            return 0;
        }
    }

    /* Wait until the start of the pulse.
     * Time out after 3 seconds so we don't block the background tasks
     */
    while (pinReadFast(pin) != value) {
        if (system_get_time() - timeoutStart > 3000000UL) {
            return 0;
        }
    }

    /* Wait until this value changes, this will be our elapsed pulse width.
     * Time out after 3 seconds so we don't block the background tasks
     */
    volatile uint32_t pulseStart = system_get_time();
    while (pinReadFast(pin) == value) {
        if (system_get_time()- timeoutStart > 3000000UL) {
            return 0;
        }
    }

    return (system_get_time() - pulseStart);
}

void HAL_pinSetFast(pin_t pin)
{
    EESP82666_Pin_Info* PIN_MAP = HAL_Pin_Map();

    pin_t gpio_pin = PIN_MAP[pin].gpio_pin;
    if(gpio_pin < 16){
        GPOS = (1 << gpio_pin);
    } else if(gpio_pin == 16){
        GP16O |= 1;
    }
}

void HAL_pinResetFast(pin_t pin)
{
    EESP82666_Pin_Info* PIN_MAP = HAL_Pin_Map();
    pin_t gpio_pin = PIN_MAP[pin].gpio_pin;
    if(gpio_pin < 16){
        GPOC = (1 << gpio_pin);
    } else if(gpio_pin == 16){
        GP16O &= ~1;
    }
}

int32_t HAL_pinReadFast(pin_t pin)
{
    EESP82666_Pin_Info* PIN_MAP = HAL_Pin_Map();
    pin_t gpio_pin = PIN_MAP[pin].gpio_pin;
    if(gpio_pin < 16){
        return GPIP(gpio_pin);
    } else if(gpio_pin == 16){
        return GP16I & 0x01;
    }
    return 0;
}

