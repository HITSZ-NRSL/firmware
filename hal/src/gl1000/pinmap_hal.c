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

/* Includes ------------------------------------------------------------------*/
#include "pinmap_hal.h"
#include "pinmap_impl.h"
#include <stddef.h>

#define NONE CHANNEL_NONE

#define GPIO_PinSource0            ((uint8_t)0x00)
#define GPIO_PinSource1            ((uint8_t)0x01)
#define GPIO_PinSource2            ((uint8_t)0x02)
#define GPIO_PinSource3            ((uint8_t)0x03)
#define GPIO_PinSource4            ((uint8_t)0x04)
#define GPIO_PinSource5            ((uint8_t)0x05)
#define GPIO_PinSource6            ((uint8_t)0x06)
#define GPIO_PinSource7            ((uint8_t)0x07)
#define GPIO_PinSource8            ((uint8_t)0x08)
#define GPIO_PinSource9            ((uint8_t)0x09)
#define GPIO_PinSource10           ((uint8_t)0x0A)
#define GPIO_PinSource11           ((uint8_t)0x0B)
#define GPIO_PinSource12           ((uint8_t)0x0C)
#define GPIO_PinSource13           ((uint8_t)0x0D)
#define GPIO_PinSource14           ((uint8_t)0x0E)
#define GPIO_PinSource15           ((uint8_t)0x0F)


/* Private typedef -----------------------------------------------------------*/

STM32_Pin_Info __PIN_MAP[TOTAL_PINS] =
{
/*
 * gpio_peripheral (GPIOA, GPIOB, GPIOC or GPIOD)
 * gpio_pin (0-15)
 * gpio_pin_source (GPIO_PinSource0 - GPIO_PinSource15)
 * adc_channel (ADC_Channel_xx or NONE. Note we don't define the peripheral because our chip only has one)
 * dac_channel (DAC_Channel_xx or NONE)
 * timer_peripheral (TIM1 - TIM5, or NONE)
 * timer_ch (1-3, or NONE)
 * pin_mode (NONE by default, can be set to OUTPUT, INPUT, or other types)
 * timer_ccr (0 by default, store the CCR value for TIM interrupt use)
 * user_property (0 by default, user variable storage)
 */
/* D0            - 00 */ { GPIOB, GPIO_PIN_10, GPIO_PinSource10, NONE, NONE, NULL, NONE, NONE, NONE, NONE },
/* D1            - 01 */ { GPIOB, GPIO_PIN_2, GPIO_PinSource2, NONE, NONE, NULL, NONE, NONE, NONE, NONE },
/* D2            - 02 */ { GPIOB, GPIO_PIN_1, GPIO_PinSource1, NONE, NONE, NULL, NONE, NONE, NONE, NONE },
/* D3            - 03 */ { GPIOB, GPIO_PIN_0, GPIO_PinSource0, ADC_CHANNEL_8, NONE, NULL, NONE, NONE, NONE, NONE },
/* D4            - 04 */ { GPIOA, GPIO_PIN_1, GPIO_PinSource1, ADC_CHANNEL_0, NONE, TIM5, TIM_CHANNEL_1, PIN_MODE_NONE, 0, 0 },
/* D5            - 05 */ { GPIOA, GPIO_PIN_0, GPIO_PinSource0, ADC_CHANNEL_1, NONE, TIM5, TIM_CHANNEL_2, PIN_MODE_NONE, 0, 0 },
/* D6            - 06 */ { GPIOB, GPIO_PIN_5, GPIO_PinSource5, NONE, NONE, TIM3, TIM_CHANNEL_2, PIN_MODE_NONE, 0, 0 },
/* D7            - 07 */ { GPIOA, GPIO_PIN_8, GPIO_PinSource8, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 },

/* D8            - 08 */ { GPIOB, GPIO_PIN_9, GPIO_PinSource9, NONE, NONE, NULL, NONE, NONE, NONE, NONE },
/* NOT USED      - 09 */ { NULL, NONE, NONE, NONE, NONE, NULL, NONE, NONE, NONE, NONE },
/* NOT USED      - 10 */ { NULL, NONE, NONE, NONE, NONE, NULL, NONE, NONE, NONE, NONE },
/* NOT USED      - 11 */ { NULL, NONE, NONE, NONE, NONE, NULL, NONE, NONE, NONE, NONE },
/* NOT USED      - 12 */ { NULL, NONE, NONE, NONE, NONE, NULL, NONE, NONE, NONE, NONE },
/* NOT USED      - 13 */ { NULL, NONE, NONE, NONE, NONE, NULL, NONE, NONE, NONE, NONE },
/* NOT USED      - 14 */ { NULL, NONE, NONE, NONE, NONE, NULL, NONE, NONE, NONE, NONE },
/* NOT USED      - 15 */ { NULL, NONE, NONE, NONE, NONE, NULL, NONE, NONE, NONE, NONE },
/* NOT USED      - 16 */ { NULL, NONE, NONE, NONE, NONE, NULL, NONE, NONE, NONE, NONE },
/* NOT USED      - 17 */ { NULL, NONE, NONE, NONE, NONE, NULL, NONE, NONE, NONE, NONE },
/* NOT USED      - 18 */ { NULL, NONE, NONE, NONE, NONE, NULL, NONE, NONE, NONE, NONE },
/* NOT USED      - 19 */ { NULL, NONE, NONE, NONE, NONE, NULL, NONE, NONE, NONE, NONE },
/* NOT USED      - 20 */ { NULL, NONE, NONE, NONE, NONE, NULL, NONE, NONE, NONE, NONE },
/* NOT USED      - 21 */ { NULL, NONE, NONE, NONE, NONE, NULL, NONE, NONE, NONE, NONE },
/* NOT USED      - 22 */ { NULL, NONE, NONE, NONE, NONE, NULL, NONE, NONE, NONE, NONE },
/* NOT USED      - 23 */ { NULL, NONE, NONE, NONE, NONE, NULL, NONE, NONE, NONE, NONE },
/* NOT USED      - 24 */ { NULL, NONE, NONE, NONE, NONE, NULL, NONE, NONE, NONE, NONE },
/* NOT USED      - 25 */ { NULL, NONE, NONE, NONE, NONE, NULL, NONE, NONE, NONE, NONE },
/* NOT USED      - 26 */ { NULL, NONE, NONE, NONE, NONE, NULL, NONE, NONE, NONE, NONE },
/* NOT USED      - 27 */ { NULL, NONE, NONE, NONE, NONE, NULL, NONE, NONE, NONE, NONE },
/* NOT USED      - 28 */ { NULL, NONE, NONE, NONE, NONE, NULL, NONE, NONE, NONE, NONE },
/* NOT USED      - 29 */ { NULL, NONE, NONE, NONE, NONE, NULL, NONE, NONE, NONE, NONE },

/* NOT USED      - 30 */ { NULL, NONE, NONE, NONE, NONE, NULL, NONE, NONE, NONE, NONE },
/* NOT USED      - 31 */ { NULL, NONE, NONE, NONE, NONE, NULL, NONE, NONE, NONE, NONE },
/* A2            - 32 */ { GPIOA, GPIO_PIN_2, GPIO_PinSource2, ADC_CHANNEL_2, NONE, TIM5, TIM_CHANNEL_3, PIN_MODE_NONE, 0, 0 },
/* A3            - 33 */ { GPIOA, GPIO_PIN_3, GPIO_PinSource3, ADC_CHANNEL_3, NONE, TIM5, TIM_CHANNEL_4, PIN_MODE_NONE, 0, 0 },
/* A4            - 34 */ { GPIOA, GPIO_PIN_4, GPIO_PinSource4, ADC_CHANNEL_4, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 },
/* A5            - 35 */ { GPIOA, GPIO_PIN_5, GPIO_PinSource5, ADC_CHANNEL_5, NONE, TIM2, TIM_CHANNEL_1, PIN_MODE_NONE, 0, 0 },
/* A6            - 36 */ { GPIOA, GPIO_PIN_6, GPIO_PinSource6, ADC_CHANNEL_6, NONE, TIM3, TIM_CHANNEL_1, PIN_MODE_NONE, 0, 0 },
/* A7            - 37 */ { GPIOA, GPIO_PIN_7, GPIO_PinSource7, ADC_CHANNEL_7, NONE, TIM3, TIM_CHANNEL_2, PIN_MODE_NONE, 0, 0 },
};


STM32_Pin_Info* HAL_Pin_Map() {
    return __PIN_MAP;
}
