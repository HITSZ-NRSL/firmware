/**
 ******************************************************************************
 * @file    usart_hal.c
 * @author  Satish Nair, Brett Walach
 * @version V1.0.0

 * @date    12-Sept-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-2015 IntoRobot Industries, Inc.  All rights reserved.

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
#include "hw_config.h"
#include "usart_hal.h"
#include "pinmap_impl.h"
#include "esp8266-hal-uart.h"
#include "core_hal_esp8266.h"

// Options for `config` argument of uart_init
#define UART_NB_BIT_5         0B00000000
#define UART_NB_BIT_6         0B00000100
#define UART_NB_BIT_7         0B00001000
#define UART_NB_BIT_8         0B00001100

#define UART_PARITY_NONE      0B00000000
#define UART_PARITY_EVEN      0B00000010
#define UART_PARITY_ODD       0B00000011

#define UART_NB_STOP_BIT_0    0B00000000
#define UART_NB_STOP_BIT_1    0B00010000
#define UART_NB_STOP_BIT_15   0B00100000
#define UART_NB_STOP_BIT_2    0B00110000

#define UART_TX_FIFO_SIZE     0x80

#define UART_8N1 ( UART_NB_BIT_8 | UART_PARITY_NONE | UART_NB_STOP_BIT_1 )

/* Private typedef -----------------------------------------------------------*/
typedef enum USART_Num_Def {
    USART_0 = 0,
    USART_1 = 1
} USART_Num_Def;


/* Private macro -------------------------------------------------------------*/
// IS_USART_CONFIG_VALID(config) - returns true for 8 data bit, any flow control, any parity, any stop byte configurations
#define IS_USART_CONFIG_VALID(CONFIG) (((CONFIG & SERIAL_VALID_CONFIG) >> 2) != 0b11)

/* Private variables ---------------------------------------------------------*/
typedef struct {
    int usart_nr;
    uint16_t usart_tx_pin;
    uint16_t usart_rx_pin;

    uart_t *usart;

    bool usart_enabled;
    bool usart_transmitting;
    int peek_char;
} ESP8266_USART_Info;

/*
 * USART mapping
 */
ESP8266_USART_Info USART_MAP[TOTAL_USARTS] =
{
    /*
     * USART_number (UART0, UART1)
     * TX pin
     * RX pin
     * <usart enabled> used internally and does not appear below
     * <usart transmitting> used internally and does not appear below
     */
    { UART0, TX, RX, NULL, false, false, -1},            // USART 0
    { UART1, TX1, 0xFF, NULL, false, false, -1 }         // USART 1
};

static ESP8266_USART_Info *usartMap[TOTAL_USARTS]; // pointer to USART_MAP[] containing USART peripheral register locations (etc)

void HAL_USART_Initial(HAL_USART_Serial serial)
{
    if(serial == HAL_USART_SERIAL1) {
        usartMap[serial] = &USART_MAP[USART_0];
    } else {
        usartMap[serial] = &USART_MAP[USART_1];
    }

    usartMap[serial]->usart_enabled = false;
    usartMap[serial]->usart_transmitting = false;
    usartMap[serial]->peek_char = -1;
}

void HAL_USART_Begin(HAL_USART_Serial serial, uint32_t baud)
{
    HAL_USART_BeginConfig(serial, baud, 0, 0); // Default serial configuration is 8N1
}

void HAL_USART_BeginConfig(HAL_USART_Serial serial, uint32_t baud, uint32_t config, void *ptr)
{
    if( true == usartMap[serial]->usart_enabled ) {
        return;
    }

    // Verify UART configuration, exit if it's invalid.
    if (!IS_USART_CONFIG_VALID(config)) {
        usartMap[serial]->usart_enabled = false;
        return;
    }

    uint32 esp8266_uart_config = 0;
    // Stop bit configuration.
    switch (config & SERIAL_STOP_BITS) {
        case 0b00: // 1 stop bit
            esp8266_uart_config |= UART_NB_STOP_BIT_1;
            break;
        case 0b01: // 2 stop bits
            esp8266_uart_config |= UART_NB_STOP_BIT_2;
            break;
    }

    // Eight / Nine data bit configuration
    if (config & SERIAL_NINE_BITS) {
        // Nine data bits, no parity.
        esp8266_uart_config |= UART_PARITY_NONE;
        esp8266_uart_config |= UART_NB_BIT_8;
    } else {
        // eight data bits, parity configuration (impacts word length)
        switch ((config & SERIAL_PARITY_BITS) >> 2) {
            case 0b00: // none
                esp8266_uart_config |= UART_PARITY_NONE;
                esp8266_uart_config |= UART_NB_BIT_8;
                break;
            case 0b01: // even
                esp8266_uart_config |= UART_PARITY_EVEN;
                esp8266_uart_config |= UART_NB_BIT_8;
                break;
            case 0b10: // odd
                esp8266_uart_config |= UART_PARITY_ODD;
                esp8266_uart_config |= UART_NB_BIT_8;
                break;
        }
    }

    EESP8266_Pin_Info* PIN_MAP = HAL_Pin_Map();
    uint16_t tx_pin = PIN_MAP[usartMap[serial]->usart_tx_pin].gpio_pin;
    uint16_t rx_pin = PIN_MAP[usartMap[serial]->usart_rx_pin].gpio_pin;

    usartMap[serial]->usart = uartBegin(usartMap[serial]->usart_nr, baud, esp8266_uart_config,
            rx_pin, tx_pin, 256);

    usartMap[serial]->usart_enabled = true;
    usartMap[serial]->usart_transmitting = false;
    usartMap[serial]->peek_char = -1;
}

void HAL_USART_End(HAL_USART_Serial serial)
{
    if(uartGetDebug() == usartMap[serial]->usart_nr) {
        uartSetDebug(UART_NO);
    }
    uartEnd(usartMap[serial]->usart);
    usartMap[serial]->usart = NULL;

    usartMap[serial]->usart_enabled = false;
    usartMap[serial]->usart_transmitting = false;
}

uint32_t HAL_USART_Write_Data(HAL_USART_Serial serial, uint8_t data)
{
    uartWrite(usartMap[serial]->usart, data);
    return 1;
}

uint32_t HAL_USART_Write_NineBitData(HAL_USART_Serial serial, uint16_t data)
{
    uartWriteBuf(usartMap[serial]->usart, (uint8_t *)&data, 2);
    return 1;
}

int32_t HAL_USART_Available_Data(HAL_USART_Serial serial)
{
    int32_t result = uartAvailable(usartMap[serial]->usart);

    if (!result) {
        optimistic_yield(10000);
    }
    return result;
}

int32_t HAL_USART_Available_Data_For_Write(HAL_USART_Serial serial)
{
    return 1;
}

int32_t HAL_USART_Read_Data(HAL_USART_Serial serial)
{
    if(HAL_USART_Available_Data(serial)) {
        return uartRead(usartMap[serial]->usart);
    }
    return -1;
}

int32_t HAL_USART_Peek_Data(HAL_USART_Serial serial)
{
    if (HAL_USART_Available_Data(serial)) {
        return uartPeek(usartMap[serial]->usart);
    }
    return -1;
}

void HAL_USART_Flush_Data(HAL_USART_Serial serial)
{
    uartFlush(usartMap[serial]->usart);
}

bool HAL_USART_Is_Enabled(HAL_USART_Serial serial)
{
    return usartMap[serial]->usart_enabled;
}

void HAL_USART_Half_Duplex(HAL_USART_Serial serial, bool Enable)
{
}

