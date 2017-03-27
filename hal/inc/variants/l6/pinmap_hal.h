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
#define TOTAL_PINS             31
#define TOTAL_ANALOG_PINS      6
#define TOTAL_DIGITAL_PINS     24
#define FIRST_ANALOG_PIN       9

#define D0 0
#define D1 1
#define D2 2
#define D3 3
#define D4 4
#define D5 5
#define D6 6
#define D7 7
#define D8 8
#define D9 9
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

#define A0  9
#define A1  10
#define A2  11
#define A3  12
#define A4  13
#define A5  14

#define PB2  D0
#define PB10 D1
#define PB11 D2
#define PB12 D3
#define PB13 D4
#define PB14 D5
#define PB15 D6

#define PA0 D7
#define PA1 D8
#define PA2 D9
#define PA3 D10
#define PA4 D11
#define PA5 D12
#define PA6 D13
#define PA7 D14
#define PA9 D15
#define PA10 D16
#define PA11 D17
#define PA12 D18
#define PA13 D19
#define PA14 D20

// lora pins
#define RESET_EN   21
#define NSS_SPI1   22
#define DIO0       23
#define DIO1       24
#define DIO2       25
#define DIO3       26
#define DIO4       27
#define DIO5       28
#define RX_EN      29

//#define A6 36
//#define A7 37

// Uart pins
#define TX  PA2
#define RX  PA3

// Uart pins
#define TX1  PA9
#define RX1  PA10

// Uart pins
#define TX2  PB10
#define RX2  PB11

//SPI pin
#define SS   PB12
#define SCK  PB13
#define MISO PB14
#define MOSI PB15

// I2C pins
#define SDA  PB10
#define SCL  PB11

#define LED_USER  D7

#define TOTAL_USARTS    3
#define TOTAL_SPIS      1
#define TOTAL_I2CS      1

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
