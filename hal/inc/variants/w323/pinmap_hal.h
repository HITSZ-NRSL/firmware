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
#define TOTAL_PINS           44
#define TOTAL_DIGITAL_PINS   10
#define FIRST_DIGITAL_PIN    0
#define TOTAL_ANALOG_PINS    14
#define FIRST_ANALOG_PIN     30


// digital pin
#define GPIO1          0
#define GPIO3          1
#define GPIO5          2
#define GPIO16         3
#define GPIO17         4
#define GPIO18         5
#define GPIO19         6
#define GPIO21         7
#define GPIO22         8
#define GPIO23         9

// analog pin
#define GPIO0          30
#define GPIO2          31
#define GPIO4          32
#define GPIO12         33
#define GPIO13         34
#define GPIO14         35
#define GPIO15         36
#define GPIO25         37
#define GPIO26         38
#define GPIO27         39
#define GPIO34         40
#define GPIO35         41
#define GPIO36         42
#define GPIO39         43

// Serial pins
#define TX  GPIO1
#define RX  GPIO3

#define TX1  GPIO17
#define RX1  GPIO16

// SPI pins
#define SS   GPIO5  //D5
#define SCK  GPIO18 //D2
#define MISO GPIO19 //D0
#define MOSI GPIO23 //D1

// SPI1 pins
#define SS1   GPIO26 //A4
#define SCK1  GPIO15 //A5
#define MISO1 GPIO14 //A6
#define MOSI1 GPIO12 //A7

// I2C pins
#define SDA  GPIO18 //D0
#define SCL  GPIO19 //D1

// I2C_1 pins
#define SDA1  GPIO25 //A2
#define SCL1  GPIO26 //A3

// The following pins are only defined for easy access during development.
// Will be removed later as they are internal I/O and users
// should not have too easy of access or bad code could do harm.

#define LED_USER  GPIO4

#define TOTAL_USARTS       2
#define TOTAL_I2CS         2
#define TOTAL_SPIS         2

#define LSBFIRST 0
#define MSBFIRST 1



/*************************************************************/
/*defined for firmware test only, not for users*/
#define INTOROBOT_TEST_PIN_DIGITAL          GPIO19
#define INTOROBOT_TEST_PIN_ADC              GPIO35
#define INTOROBOT_TEST_PIN_DAC              GPIO25



#ifdef __cplusplus
}
#endif

#endif  /* PINMAP_HAL_H_ */
