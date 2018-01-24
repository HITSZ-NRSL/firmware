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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef PINMAP_HAL_H_
#define PINMAP_HAL_H_

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>
#include "intorobot_config.h"


#ifdef __cplusplus
extern "C" {
#endif

/* Exported types ------------------------------------------------------------*/

typedef uint16_t pin_t;

typedef enum PinMode {
    INPUT,
    OUTPUT,
    INPUT_PULLUP,
    INPUT_PULLDOWN,     // PULLDOWN only possible for pin16
    //Used internally begin
    AF_OUTPUT_PUSHPULL, //Used internally for Alternate Function Output PushPull(TIM, UART, SPI etc)
    AF_OUTPUT_DRAIN,    //Used internally for Alternate Function Output Drain(I2C etc). External pullup resistors required.
    AN_INPUT,           //Used internally for ADC Input
    AN_OUTPUT,          //Used internally for DAC Output
    //Used internally end
    PIN_MODE_NONE=0xFF
} PinMode;

typedef enum {
    PF_NONE,
    PF_DIO,
    PF_TIMER,
    PF_ADC,
    PF_DAC
} PinFunction;

PinFunction HAL_Validate_Pin_Function(pin_t pin, PinFunction pinFunction);

typedef struct EESP8266_Pin_Info  EESP8266_Pin_Info;

EESP8266_Pin_Info* HAL_Pin_Map(void);

/* Exported macros -----------------------------------------------------------*/

/*
* Pin mapping. Borrowed from Wiring
*/

#define TOTAL_PINS              31
#define FIRST_DIGITAL_PIN       0
#define TOTAL_DIGITAL_PINS      11
#define FIRST_ANALOG_PIN        30
#define TOTAL_ANALOG_PINS       1

#define GPIO0  0
#define GPIO1  1
#define GPIO2  2
#define GPIO3  3
#define GPIO4  4
#define GPIO5  5
#define GPIO12 6
#define GPIO13 7
#define GPIO14 8
#define GPIO15 9
#define GPIO16 10

#define ADC    30

// Serial pin
#define TX   GPIO1
#define RX   GPIO3

#define TXD  TX
#define RXD  RX

// Serial1 pin
#define TX1   GPIO2

#define TXD1  TX1

// SPI pins
#define SS   GPIO15
#define SCK  GPIO14
#define MISO GPIO12
#define MOSI GPIO13

// I2C pins
#define SDA  GPIO4
#define SCL  GPIO2

#define LED_USER    GPIO16
#define LED_BUILTIN LED_USER


// The following pins are only defined for easy access during development.
// Will be removed later as they are internal I/O and users
// should not have too easy of access or bad code could do harm.
#define TOTAL_USARTS        2
#define TOTAL_SPIS          1
#define TOTAL_I2CS          1

#define LSBFIRST 0
#define MSBFIRST 1

/*************************************************************/
/*defined for firmware test only, not for users*/
#define INTOROBOT_TEST_PIN_DIGITAL          GPIO2
#define INTOROBOT_TEST_PIN_ADC              ADC


#ifdef __cplusplus
}
#endif

#endif  /* PINMAP_HAL_H_ */
