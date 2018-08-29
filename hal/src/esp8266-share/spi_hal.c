/**
 ******************************************************************************
 * @file    spi_hal.c
 * @author  Matthew McGowan
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
#include "spi_hal.h"
#include "pinmap_impl.h"
#include "molmc_log.h"
#include "esp8266-hal-spi.h"


static bool SPI_Enabled = false;
static bool SPI_Bit_Order_Set = false;
static bool SPI_Data_Mode_Set = false;
static bool SPI_Clock_Divider_Set = false;


void HAL_SPI_Initial(HAL_SPI_Interface spi)
{
    if(spi != HAL_SPI_INTERFACE1)
    {
        return;
    }
    spi_selectSet();
}

void HAL_SPI_Begin(HAL_SPI_Interface spi, uint16_t pin)
{
    if(spi != HAL_SPI_INTERFACE1)
    {
        return;
    }

    if(!SPI_Enabled)
    {
        EESP8266_Pin_Info *PIN_MAP = HAL_Pin_Map();
        pin_t sck_pin = PIN_MAP[SCK].gpio_pin;
        pin_t miso_pin = PIN_MAP[MISO].gpio_pin;
        pin_t mosi_pin = PIN_MAP[MOSI].gpio_pin;
        pin_t ss_pin = PIN_MAP[SS].gpio_pin;

        if(!SPI_Data_Mode_Set)
        {
            spi_setDataMode(ESP8266_SPI_MODE0);
        }

        if(!SPI_Bit_Order_Set)
        {
            spi_setBitOrder(MSBFIRST);
        }

        if(!SPI_Clock_Divider_Set)
        {
            spi_setClockDivider(ESP8266_SPI_CLOCK_DIV16);
        }

        spi_begin(sck_pin,miso_pin,mosi_pin,ss_pin);
        SPI_Enabled = true;
    }
}

void HAL_SPI_End(HAL_SPI_Interface spi)
{
    spi_end();
}

void HAL_SPI_Set_Bit_Order(HAL_SPI_Interface spi, uint8_t bitOrder)
{
    spi_setBitOrder(bitOrder);
    SPI_Bit_Order_Set = true;
}

void HAL_SPI_Set_Data_Mode(HAL_SPI_Interface spi, uint8_t dataMode)
{
    switch(dataMode)
    {
        case SPI_MODE0:
            spi_setDataMode(ESP8266_SPI_MODE0);
            break;
        case SPI_MODE1:
            spi_setDataMode(ESP8266_SPI_MODE1);
            break;
        case SPI_MODE2:
            spi_setDataMode(ESP8266_SPI_MODE2);
            break;
        case SPI_MODE3:
        default:
            spi_setDataMode(ESP8266_SPI_MODE3);
            break;
    }
    SPI_Data_Mode_Set = true;
}

void HAL_SPI_Set_Clock_Divider(HAL_SPI_Interface spi, uint8_t rate)
{
    switch(rate)
    {
        case SPI_CLOCK_DIV2:
            spi_setClockDivider(ESP8266_SPI_CLOCK_DIV2);
            break;
        case SPI_CLOCK_DIV4:
            spi_setClockDivider(ESP8266_SPI_CLOCK_DIV4);
            break;
        case SPI_CLOCK_DIV8:
            spi_setClockDivider(ESP8266_SPI_CLOCK_DIV8);
            break;
        case SPI_CLOCK_DIV16:
            spi_setClockDivider(ESP8266_SPI_CLOCK_DIV16);
            break;
        case SPI_CLOCK_DIV32:
            spi_setClockDivider(ESP8266_SPI_CLOCK_DIV32);
            break;
        case SPI_CLOCK_DIV64:
            spi_setClockDivider(ESP8266_SPI_CLOCK_DIV64);
            break;
        case SPI_CLOCK_DIV128:
        case SPI_CLOCK_DIV256:
        default:
            spi_setClockDivider(ESP8266_SPI_CLOCK_DIV128);
            break;
    }
    SPI_Clock_Divider_Set = true;
}

uint16_t HAL_SPI_Send_Receive_Data(HAL_SPI_Interface spi, uint16_t data)
{
    return spi_transfer((uint8_t)data);
}

bool HAL_SPI_Is_Enabled(HAL_SPI_Interface spi)
{
    return false;
}

void HAL_SPI_DMA_Transfer(HAL_SPI_Interface spi, void* tx_buffer, void* rx_buffer, uint32_t length, HAL_SPI_DMA_UserCallback userCallback)
{
}

void HAL_SPI_Info(HAL_SPI_Interface spi, hal_spi_info_t* info, void* reserved)
{
}

void HAL_SPI_Set_Callback_On_Select(HAL_SPI_Interface spi, HAL_SPI_Select_UserCallback cb, void* reserved)
{
}

void HAL_SPI_DMA_Transfer_Cancel(HAL_SPI_Interface spi)
{
}

int32_t HAL_SPI_DMA_Transfer_Status(HAL_SPI_Interface spi, HAL_SPI_TransferStatus* st)
{
    return 0;
}

