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

#define GPIO_0                      0     // GPIO_NUM_0
#define GPIO_1                      1     // GPIO_NUM_1
#define GPIO_2                      2     // GPIO_NUM_2
#define GPIO_3                      3     // GPIO_NUM_3
#define GPIO_4                      4     // GPIO_NUM_4
#define GPIO_5                      5     // GPIO_NUM_5
#define GPIO_6                      6     // GPIO_NUM_6
#define GPIO_7                      7     // GPIO_NUM_7
#define GPIO_8                      8     // GPIO_NUM_8
#define GPIO_9                      9     // GPIO_NUM_9
#define GPIO_10                     10
#define GPIO_11                     11
#define GPIO_12                     12
#define GPIO_13                     13
#define GPIO_14                     14
#define GPIO_15                     15
#define GPIO_16                     16
#define GPIO_17                     17
#define GPIO_18                     18
#define GPIO_19                     19
#define GPIO_20                     20
#define GPIO_21                     21
#define GPIO_22                     22
#define GPIO_23                     23
#define GPIO_24                     24
#define GPIO_25                     25
#define GPIO_26                     26
#define GPIO_27                     27
#define GPIO_28                     28
#define GPIO_29                     29
#define GPIO_30                     30
#define GPIO_31                     31
#define GPIO_32                     32
#define GPIO_33                     33
#define GPIO_34                     34 /* input mode only */
#define GPIO_35                     35 /* input mode only */
#define GPIO_36                     36 /* input mode only */
#define GPIO_37                     37 /* input mode only */
#define GPIO_38                     38 /* input mode only */
#define GPIO_39                     39 /* input mode only */


#define ADC_CHANNEL  ((uint8_t)0x01)
#define DAC_CHANNEL  ((uint8_t)0x01)

#define ESP32_TIMER_0  0
#define ESP32_TIMER_1  1
#define ESP32_TIMER_2  2
#define ESP32_TIMER_3  3

#define TIMER_PWM_CHANNEL_0 0
#define TIMER_PWM_CHANNEL_1 1
#define TIMER_PWM_CHANNEL_2 2
#define TIMER_PWM_CHANNEL_3 3
#define TIMER_PWM_CHANNEL_4 4
#define TIMER_PWM_CHANNEL_5 5
#define TIMER_PWM_CHANNEL_6 6
#define TIMER_PWM_CHANNEL_7 7
#define TIMER_PWM_CHANNEL_8 8
#define TIMER_PWM_CHANNEL_9 9
#define TIMER_PWM_CHANNEL_10 10
#define TIMER_PWM_CHANNEL_11 11
#define TIMER_PWM_CHANNEL_12 12
#define TIMER_PWM_CHANNEL_13 13
#define TIMER_PWM_CHANNEL_14 14
#define TIMER_PWM_CHANNEL_15 15

/* Private typedef -----------------------------------------------------------*/

ESP32_Pin_Info __PIN_MAP[TOTAL_PINS] =
{
/*
 * gpio_pin (0-16)
 * gpio_pin_source (GPIO_PinSource0 - GPIO_PinSource15)
 * adc_channel
 * dac_channel
 * timer_peripheral (TIM1 - TIM5, or NONE)
 * timer_ch (1-3, or NONE)
 * pin_mode (NONE by default, can be set to OUTPUT, INPUT, or other types)
 */
/* D0               - 00 */ { GPIO_19, NONE, NONE, NONE, ESP32_TIMER_0, TIMER_PWM_CHANNEL_0, PIN_MODE_NONE, 0 },
/* D1               - 01 */ { GPIO_23, NONE, NONE, NONE, ESP32_TIMER_0, TIMER_PWM_CHANNEL_1, PIN_MODE_NONE, 0 },
/* D2               - 02 */ { GPIO_18, NONE, NONE, NONE, ESP32_TIMER_1, TIMER_PWM_CHANNEL_2, PIN_MODE_NONE, 0 },
/* D3               - 03 */ { GPIO_17, NONE, NONE, NONE, ESP32_TIMER_1, TIMER_PWM_CHANNEL_3, PIN_MODE_NONE, 0 },
/* D4               - 04 */ { GPIO_16, NONE, NONE, NONE, ESP32_TIMER_2, TIMER_PWM_CHANNEL_4, PIN_MODE_NONE, 0 },
/* D5               - 05 */ { GPIO_5,  NONE, NONE, NONE, ESP32_TIMER_2, TIMER_PWM_CHANNEL_5, PIN_MODE_NONE, 0 },
/* D6               - 06 */ { GPIO_4,  NONE, NONE, NONE, ESP32_TIMER_3, TIMER_PWM_CHANNEL_6, PIN_MODE_NONE, 0 },
/* D7               - 07 */ { GPIO_4,  NONE, NONE, NONE, ESP32_TIMER_3, TIMER_PWM_CHANNEL_6, PIN_MODE_NONE, 0 },
/* TXD              - 08 */ { GPIO_1,  NONE, NONE, NONE, NONE, NONE, PIN_MODE_NONE, 0 },
/* RXD              - 09 */ { GPIO_3,  NONE, NONE, NONE, NONE, NONE, PIN_MODE_NONE, 0 },
/* NOT USED         - 10 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE, 0 },
/* NOT USED         - 11 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE, 0 },
/* NOT USED         - 12 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE, 0 },
/* NOT USED         - 13 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE, 0 },
/* NOT USED         - 14 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE, 0 },
/* NOT USED         - 15 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE, 0 },
/* NOT USED         - 16 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE, 0 },
/* NOT USED         - 17 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE, 0 },
/* NOT USED         - 18 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE, 0 },
/* NOT USED         - 19 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE, 0 },
/* NOT USED         - 20 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE, 0 },
/* NOT USED         - 21 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE, 0 },
/* NOT USED         - 22 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE, 0 },
/* NOT USED         - 23 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE, 0 },
/* NOT USED         - 24 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE, 0 },
/* NOT USED         - 25 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE, 0 },
/* NOT USED         - 26 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE, 0 },
/* NOT USED         - 27 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE, 0 },
/* NOT USED         - 28 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE, 0 },
/* NOT USED         - 29 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE, 0 },
/* A0               - 30 */ { GPIO_36, NONE, ADC_CHANNEL, NONE, NONE, NONE, PIN_MODE_NONE, 0 },
/* A1               - 31 */ { GPIO_39, NONE, ADC_CHANNEL, NONE, NONE, NONE, PIN_MODE_NONE, 0 },
/* A2               - 32 */ { GPIO_35, NONE, ADC_CHANNEL, NONE, NONE, NONE, PIN_MODE_NONE, 0 },
/* A3               - 33 */ { GPIO_25, NONE, ADC_CHANNEL, DAC_CHANNEL, ESP32_TIMER_3, TIMER_PWM_CHANNEL_7, PIN_MODE_NONE, 0 },
/* A4               - 34 */ { GPIO_26, NONE, ADC_CHANNEL, DAC_CHANNEL, ESP32_TIMER_0, TIMER_PWM_CHANNEL_8, PIN_MODE_NONE, 0 },
/* A5               - 35 */ { GPIO_14, NONE, ADC_CHANNEL, NONE, ESP32_TIMER_0, TIMER_PWM_CHANNEL_9, PIN_MODE_NONE, 0 },
/* A6               - 36 */ { GPIO_12, NONE, ADC_CHANNEL, NONE, ESP32_TIMER_1, TIMER_PWM_CHANNEL_10, PIN_MODE_NONE, 0 },
/* A7               - 37 */ { GPIO_15, NONE, ADC_CHANNEL, NONE, ESP32_TIMER_1, TIMER_PWM_CHANNEL_11, PIN_MODE_NONE, 0 },
/* A8               - 38 */ { GPIO_13, NONE, ADC_CHANNEL, NONE, ESP32_TIMER_2, TIMER_PWM_CHANNEL_12, PIN_MODE_NONE, 0 },
/* A9               - 39 */ { GPIO_2,  NONE, ADC_CHANNEL, NONE, ESP32_TIMER_2, TIMER_PWM_CHANNEL_13, PIN_MODE_NONE, 0 },
/* LIGHT_SENSOR_UC  - 40 */ { GPIO_34, NONE, ADC_CHANNEL, NONE, NONE, NONE, PIN_MODE_NONE, 0 },
};

ESP32_Pin_Info* HAL_Pin_Map() {
    return __PIN_MAP;
}

