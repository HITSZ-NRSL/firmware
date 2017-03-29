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

#ifndef PINMAP_IMPL_H
#define PINMAP_IMPL_H

#include "hw_config.h"
#include "pinmap_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct EESP32_Pin_Info {
    pin_t gpio_pin;
    uint8_t gpio_pin_source;
    uint8_t adc_channel;
    uint8_t dac_channel;
    uint8_t timer_peripheral;
    PinMode pin_mode;
} ESP32_Pin_Info;

ESP32_Pin_Info* HAL_Pin_Map(void);

extern void HAL_GPIO_Save_Pin_Mode(PinMode mode);
extern PinMode HAL_GPIO_Recall_Pin_Mode();

#define CHANNEL_NONE ((uint8_t)(0xFF))
#define ADC_CHANNEL_NONE CHANNEL_NONE
#define DAC_CHANNEL_NONE CHANNEL_NONE

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



#ifdef __cplusplus
}
#endif

#endif /* PINMAP_IMPL_H */

