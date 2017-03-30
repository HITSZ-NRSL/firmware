/*
 Arduino.h - Main include file for the Arduino SDK
 Copyright (c) 2005-2013 Arduino Team.  All right reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef MAIN_ESP32_HAL_GPIO_H_
#define MAIN_ESP32_HAL_GPIO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "esp32-hal.h"

#define LOW               0x0
#define HIGH              0x1

//GPIO FUNCTIONS
#define ESP32_INPUT             0x01
#define ESP32_OUTPUT            0x02
#define ESP32_PULLUP            0x04
#define ESP32_INPUT_PULLUP      0x05
#define ESP32_PULLDOWN          0x08
#define ESP32_INPUT_PULLDOWN    0x09
#define ESP32_OPEN_DRAIN        0x10
#define ESP32_OUTPUT_OPEN_DRAIN 0x12
#define ESP32_SPECIAL           0xF0
#define ESP32_FUNCTION_1        0x00
#define ESP32_FUNCTION_2        0x20
#define ESP32_FUNCTION_3        0x40
#define ESP32_FUNCTION_4        0x60
#define ESP32_FUNCTION_5        0x80
#define ESP32_FUNCTION_6        0xA0
#define ESP32_ANALOG            0xC0

//Interrupt Modes
#define ESP32_DISABLED  0x00
#define ESP32_RISING    0x01
#define ESP32_FALLING   0x02
#define ESP32_CHANGE    0x03
#define ESP32_ONLOW     0x04
#define ESP32_ONHIGH    0x05
#define ESP32_ONLOW_WE  0x0C
#define ESP32_ONHIGH_WE 0x0D

typedef struct {
    uint8_t reg;      /*!< GPIO register offset from DR_REG_IO_MUX_BASE */
    int8_t rtc;       /*!< RTC GPIO number (-1 if not RTC GPIO pin) */
    int8_t adc;       /*!< ADC Channel number (-1 if not ADC pin) */
    int8_t touch;     /*!< Touch Channel number (-1 if not Touch pin) */
} esp32_gpioMux_t;

extern const esp32_gpioMux_t esp32_gpioMux[40];
extern const int8_t esp32_adc2gpio[20];

#define digitalPinIsValid(pin)          ((pin) < 40 && esp32_gpioMux[(pin)].reg)
#define digitalPinCanOutput(pin)        ((pin) < 34 && esp32_gpioMux[(pin)].reg)
#define digitalPinToRtcPin(pin)         (((pin) < 40)?esp32_gpioMux[(pin)].rtc:-1)
#define digitalPinToAnalogChannel(pin)  (((pin) < 40)?esp32_gpioMux[(pin)].adc:-1)
#define digitalPinToTouchChannel(pin)   (((pin) < 40)?esp32_gpioMux[(pin)].touch:-1)
#define digitalPinToDacChannel(pin)     (((pin) == 25)?0:((pin) == 26)?1:-1)


typedef void (*voidFuncPtr)(void);

extern void IRAM_ATTR __pinMode(uint8_t pin, uint8_t mode);
extern void IRAM_ATTR __digitalWrite(uint8_t pin, uint8_t val);
extern int IRAM_ATTR __digitalRead(uint8_t pin);
extern void __attachInterrupt(uint8_t pin, voidFuncPtr userFunc, int intr_type);
extern void __detachInterrupt(uint8_t pin);


#ifdef __cplusplus
}
#endif

#endif /* MAIN_ESP32_HAL_GPIO_H_ */
