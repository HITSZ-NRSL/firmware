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
#define GPIO_34                     34    /* input mode only */
#define GPIO_35                     35    /* input mode only */
#define GPIO_36                     36    /* input mode only */
#define GPIO_37                     37    /* input mode only */
#define GPIO_38                     38    /* input mode only */
#define GPIO_39                     39    /* input mode only */

/* #define GPIO_PinSource0            ((uint8_t)0x00) */
/* #define GPIO_PinSource1            ((uint8_t)0x01) */
/* #define GPIO_PinSource2            ((uint8_t)0x02) */
/* #define GPIO_PinSource3            ((uint8_t)0x03) */
/* #define GPIO_PinSource4            ((uint8_t)0x04) */
/* #define GPIO_PinSource5            ((uint8_t)0x05) */
/* #define GPIO_PinSource6            ((uint8_t)0x06) */
/* #define GPIO_PinSource7            ((uint8_t)0x07) */
/* #define GPIO_PinSource8            ((uint8_t)0x08) */
/* #define GPIO_PinSource9            ((uint8_t)0x09) */
/* #define GPIO_PinSource10           ((uint8_t)0x0A) */
/* #define GPIO_PinSource11           ((uint8_t)0x0B) */
/* #define GPIO_PinSource12           ((uint8_t)0x0C) */
/* #define GPIO_PinSource13           ((uint8_t)0x0D) */
/* #define GPIO_PinSource14           ((uint8_t)0x0E) */
/* #define GPIO_PinSource15           ((uint8_t)0x0F) */
/* #define GPIO_PinSource16           ((uint8_t)0x10) */

/* #define ADC_Channel_0  ((uint8_t)0x00) */
/* #define TIM1           ((uint8_t)0x00) */

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
/*             - 00 */ { GPIO_1, NONE, NONE, NONE, PIN_MODE_NONE },
/*             - 01 */ { GPIO_3, NONE, NONE, NONE, PIN_MODE_NONE },
/*             - 02 */ { GPIO_5, NONE, NONE, NONE, PIN_MODE_NONE },
/*             - 03 */ { GPIO_16, NONE, NONE, NONE, PIN_MODE_NONE },
/*             - 04 */ { GPIO_17, NONE, NONE, NONE, PIN_MODE_NONE },
/*             - 05 */ { GPIO_18, NONE, NONE, NONE, PIN_MODE_NONE },
/*             - 06 */ { GPIO_19, NONE, NONE, NONE, PIN_MODE_NONE },
/*             - 07 */ { GPIO_21, NONE, NONE, NONE, PIN_MODE_NONE },
/*             - 08 */ { GPIO_22, NONE, NONE, NONE, PIN_MODE_NONE },
/*             - 09 */ { GPIO_23, NONE, NONE, NONE, PIN_MODE_NONE },
/* NOT USED    - 10 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE },
/* NOT USED    - 11 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE },
/* NOT USED    - 12 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE },
/* NOT USED    - 13 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE },
/* NOT USED    - 14 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE },
/* NOT USED    - 15 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE },
/* NOT USED    - 16 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE },
/* NOT USED    - 17 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE },
/* NOT USED    - 18 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE },
/* NOT USED    - 19 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE },
/* NOT USED    - 20 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE },
/* NOT USED    - 21 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE },
/* NOT USED    - 22 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE },
/* NOT USED    - 23 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE },
/* NOT USED    - 24 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE },
/* NOT USED    - 25 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE },
/* NOT USED    - 26 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE },
/* NOT USED    - 27 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE },
/* NOT USED    - 28 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE },
/* NOT USED    - 29 */ { NONE, NONE, NONE, NONE, PIN_MODE_NONE },
/*             - 30 */ { GPIO_0,  NONE, NONE, NONE, PIN_MODE_NONE },
/*             - 31 */ { GPIO_2,  NONE, NONE, NONE, PIN_MODE_NONE },
/*             - 32 */ { GPIO_4,  NONE, NONE, NONE, PIN_MODE_NONE },
/*             - 33 */ { GPIO_12, NONE, NONE, NONE, PIN_MODE_NONE },
/*             - 34 */ { GPIO_13, NONE, NONE, NONE, PIN_MODE_NONE },
/*             - 35 */ { GPIO_14, NONE, NONE, NONE, PIN_MODE_NONE },
/*             - 36 */ { GPIO_15, NONE, NONE, NONE, PIN_MODE_NONE },
/*             - 37 */ { GPIO_25, NONE, NONE, NONE, PIN_MODE_NONE },
/*             - 38 */ { GPIO_26, NONE, NONE, NONE, PIN_MODE_NONE },
/*             - 39 */ { GPIO_27, NONE, NONE, NONE, PIN_MODE_NONE },
/*             - 40 */ { GPIO_34, NONE, NONE, NONE, PIN_MODE_NONE },
/*             - 41 */ { GPIO_35, NONE, NONE, NONE, PIN_MODE_NONE },
/*             - 42 */ { GPIO_36, NONE, NONE, NONE, PIN_MODE_NONE },
/*             - 43 */ { GPIO_39, NONE, NONE, NONE, PIN_MODE_NONE },
};

ESP32_Pin_Info* HAL_Pin_Map() {
    return __PIN_MAP;
}
