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
    INPUT_PULLDOWN,
    //Used internally begin
    OPEN_DRAIN,
    OUTPUT_OPEN_DRAIN,
    SPECIAL,
    FUNCTION_1,
    FUNCTION_2,
    FUNCTION_3,
    FUNCTION_4,
    FUNCTION_5,
    FUNCTION_6,
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

typedef struct EESP32_Pin_Info  EESP32_Pin_Info;

EESP32_Pin_Info* HAL_Pin_Map(void);


/* Exported macros -----------------------------------------------------------*/

/*
* Pin mapping. Borrowed from Wiring
*/

#define TOTAL_PINS           41
#define TOTAL_DIGITAL_PINS   8
#define FIRST_DIGITAL_PIN    0
#define TOTAL_ANALOG_PINS    11
#define FIRST_ANALOG_PIN     30

#define D0  0
#define D1  1
#define D2  2
#define D3  3
#define D4  4
#define D5  5
#define D6  6
#define D7  7

#define A0  30
#define A1  31
#define A2  32
#define A3  33
#define A4  34
#define A5  35
#define A6  36
#define A7  37
#define A8  38
#define A9  39

#define LIGHT_SENSOR_UC 40
//Serial pins
#define TX  1
#define RX  3

// Serial1 pin
#define TX1  D3
#define RX1  D4

// SPI pins
#define SS   D5
#define SCK  D2
#define MISO D3
#define MOSI D4

// SPI1 pins
#define SS1   A7
#define SCK1  A5
#define MISO1 A6
#define MOSI1 A8

// Wire pins
#define SDA  D1
#define SCL  D0

// Wire1 pins
#define SDA1  A2
#define SCL1  A3

// The following pins are only defined for easy access during development.
// Will be removed later as they are internal I/O and users
// should not have too easy of access or bad code could do harm.

#define LED_USER  D6

#define TOTAL_USARTS    2
#define TOTAL_SPIS      2
#define TOTAL_I2CS      2


#define LSBFIRST 0
#define MSBFIRST 1


/*************************************************************/
/*defined for firmware test only, not for users*/
#define INTOROBOT_TEST_PIN_DIGITAL          D0
#define INTOROBOT_TEST_PIN_ADC              A2
#define INTOROBOT_TEST_PIN_DAC              A3


#ifdef __cplusplus
}
#endif

#endif  /* PINMAP_HAL_H_ */
