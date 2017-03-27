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
    AF_OUTPUT_PUSHPULL, //Used internally for Alternate Function Output PushPull(TIM, UART, SPI etc)
    AF_OUTPUT_DRAIN,    //Used internally for Alternate Function Output Drain(I2C etc). External pullup resistors required.
    AN_INPUT,           //Used internally for ADC Input
    AN_OUTPUT,          //Used internally for DAC Output
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

typedef struct STM32_Pin_Info  STM32_Pin_Info;

STM32_Pin_Info* HAL_Pin_Map(void);

/* Exported macros -----------------------------------------------------------*/

/*
* Pin mapping. Borrowed from Wiring
*/
#define TOTAL_PINS             47
#define TOTAL_ANALOG_PINS      16
#define TOTAL_DIGITAL_PINS     26
#define FIRST_ANALOG_PIN       30

#define D0  0
#define D1  1
#define D2  2
#define D3  3
#define D4  4
#define D5  5
#define D6  6
#define D7  7
#define D8  8
#define D9  9
#define D10 10
#define D11 11
#define D12 12
#define D13 13
#define D14 14
#define D15 15
#define D16 16
#define D17 17
#define D18 18
#define D19 19
#define D20 20
#define D21 21
#define D22 22
#define D23 23
#define D24 24
#define D25 25


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
#define A10 40
#define A11 41
#define A12 42
#define A13 43
#define A14 44
#define A15 45


//uart pin
#define TX  A7
#define RX  A8

//uart1 pin
#define TX1 D23
#define RX1 D22

//uart2 pin
#define TX2 D13
#define RX2 D12

// SPI pins
#define SS   D21
#define SCK  D20
#define MISO D19
#define MOSI D18

#define SS1    A9
#define SCK1   A10
#define MISO1  A11
#define MOSI1  A12


// I2C pins
#define SDA  D2
#define SCL  D3

// I2C1 pins
#define SDA1  D22
#define SCL1  D23

// The following pins are only defined for easy access during development.
// Will be removed later as they are internal I/O and users
// should not have too easy of access or bad code could do harm.
#define LED_USER  D7

#define TOTAL_USARTS    3
#define TOTAL_SPIS      2
#define TOTAL_I2CS      2
#define TOTAL_CANS      1

#define LSBFIRST 0
#define MSBFIRST 1

/*************************************************************/
/*defined for firmware test only, not for users*/
#define INTOROBOT_TEST_PIN_DIGITAL          D0
#define INTOROBOT_TEST_PIN_ADC              A0



#ifdef __cplusplus
}
#endif

#endif  /* PINMAP_HAL_H_ */
