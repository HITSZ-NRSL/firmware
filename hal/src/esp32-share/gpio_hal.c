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

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_attr.h"
#include "esp_intr.h"
#include "rom/ets_sys.h"
#include "rom/gpio.h"
#include "soc/gpio_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/gpio_struct.h"
#include "soc/io_mux_reg.h"

PinMode digitalPinModeSaved = PIN_MODE_NONE;

#define ETS_GPIO_INUM       12

typedef void (*voidFuncPtr)(void);
static voidFuncPtr __pinInterruptHandlers[GPIO_PIN_COUNT] = {0,};

#include "driver/rtc_io.h"

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
    EESP32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    pin_t gpio_pin = PIN_MAP[pin].gpio_pin;

    uint32_t rtc_reg = rtc_gpio_desc[gpio_pin].reg;
    if(setMode == AN_INPUT) {
        PIN_MAP[pin].pin_mode = AN_INPUT;
        if(!rtc_reg) {
            return;//not rtc pin
        }
        //lock rtc
        uint32_t reg_val = ESP_REG(rtc_reg);
        if(reg_val & rtc_gpio_desc[gpio_pin].mux){
            return;//already in adc mode
        }
        reg_val &= ~(
                (RTC_IO_TOUCH_PAD1_FUN_SEL_V << rtc_gpio_desc[gpio_pin].func)
                |rtc_gpio_desc[gpio_pin].ie
                |rtc_gpio_desc[gpio_pin].pullup
                |rtc_gpio_desc[gpio_pin].pulldown);
        ESP_REG(RTC_GPIO_ENABLE_W1TC_REG) = (1 << (rtc_gpio_desc[gpio_pin].rtc_num + RTC_GPIO_ENABLE_W1TC_S));
        ESP_REG(rtc_reg) = reg_val | rtc_gpio_desc[gpio_pin].mux;
        //unlock rtc
        ESP_REG(DR_REG_IO_MUX_BASE + esp32_gpioMux[gpio_pin].reg) = ((uint32_t)2 << MCU_SEL_S) | ((uint32_t)2 << FUN_DRV_S) | FUN_IE;
        return;
    }

    //RTC pins PULL settings
    if(rtc_reg) {
        //lock rtc
        ESP_REG(rtc_reg) = ESP_REG(rtc_reg) & ~(rtc_gpio_desc[gpio_pin].mux);
        if(setMode & INPUT_PULLUP) {
            ESP_REG(rtc_reg) = (ESP_REG(rtc_reg) | rtc_gpio_desc[gpio_pin].pullup) & ~(rtc_gpio_desc[gpio_pin].pulldown);
        } else if(setMode & INPUT_PULLDOWN) {
            ESP_REG(rtc_reg) = (ESP_REG(rtc_reg) | rtc_gpio_desc[gpio_pin].pulldown) & ~(rtc_gpio_desc[gpio_pin].pullup);
        } else {
            ESP_REG(rtc_reg) = ESP_REG(rtc_reg) & ~(rtc_gpio_desc[gpio_pin].pullup | rtc_gpio_desc[gpio_pin].pulldown);
        }
        //unlock rtc
    }

    uint32_t pinFunction = 0, pinControl = 0;

    //lock gpio
    if(setMode & INPUT) {
        PIN_MAP[pin].pin_mode = INPUT;
        if(gpio_pin < 32) {
            GPIO.enable_w1tc = ((uint32_t)1 << gpio_pin);
        } else {
            GPIO.enable1_w1tc.val = ((uint32_t)1 << (gpio_pin - 32));
        }
    } else if(setMode & OUTPUT) {
        PIN_MAP[pin].pin_mode = OUTPUT;
        if(gpio_pin > 33){
            //unlock gpio
            return;//pins above 33 can be only inputs
        } else if(gpio_pin < 32) {
            GPIO.enable_w1ts = ((uint32_t)1 << gpio_pin);
        } else {
            GPIO.enable1_w1ts.val = ((uint32_t)1 << (gpio_pin - 32));
        }
    } else if(setMode & INPUT_PULLUP) {
        PIN_MAP[pin].pin_mode = INPUT_PULLUP;
        pinFunction |= FUN_PU;
    } else if(setMode & INPUT_PULLDOWN) {
        PIN_MAP[pin].pin_mode = INPUT_PULLDOWN;
        pinFunction |= FUN_PD;
    }

    pinFunction |= ((uint32_t)2 << FUN_DRV_S);//what are the drivers?
    pinFunction |= FUN_IE;//input enable but required for output as well?

    if(setMode & (INPUT | OUTPUT)) {
        pinFunction |= ((uint32_t)2 << MCU_SEL_S);
    } else if(setMode == SPECIAL) {
        pinFunction |= ((uint32_t)(((gpio_pin)==1||(gpio_pin)==3)?0:1) << MCU_SEL_S);
    } else {
        pinFunction |= ((uint32_t)(setMode >> 5) << MCU_SEL_S);
    }

    ESP_REG(DR_REG_IO_MUX_BASE + esp32_gpioMux[gpio_pin].reg) = pinFunction;

    if(setMode & OPEN_DRAIN) {
        pinControl = (1 << GPIO_PIN0_PAD_DRIVER_S);
    }

    GPIO.pin[gpio_pin].val = pinControl;
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

    if(value) {
        if(gpio_pin < 32) {
            GPIO.out_w1ts = ((uint32_t)1 << gpio_pin);
        } else  if(pin < 34){
            GPIO.out1_w1ts.val = ((uint32_t)1 << (gpio_pin - 32));
        }
    } else {
        if(gpio_pin < 32) {
            GPIO.out_w1tc = ((uint32_t)1 << gpio_pin);
        } else if(pin< 34){
            GPIO.out1_w1tc.val = ((uint32_t)1 << (gpio_pin - 32));
        }
    }
}

/*
 * @brief Reads the value of a GPIO pin. Should return either 1 (HIGH) or 0 (LOW).
 */
int32_t HAL_GPIO_Read(uint16_t pin)
{
    EESP32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    pin_t gpio_pin = PIN_MAP[pin].gpio_pin;

    if(gpio_pin < 32) {
        return (GPIO.in >> gpio_pin) & 0x1;
    } else if(pin < 40){
        return (GPIO.in1.val >> (gpio_pin - 32)) & 0x1;
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

