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

#include "esp32-hal-spi.h"
#include "esp32-hal.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "rom/ets_sys.h"
#include "esp_attr.h"
#include "esp_intr.h"
#include "rom/gpio.h"
#include "soc/spi_reg.h"
#include "soc/spi_struct.h"
#include "soc/io_mux_reg.h"
#include "soc/gpio_sig_map.h"
#include "soc/dport_reg.h"


struct spi_struct_t {
    spi_dev_t * dev;
    xSemaphoreHandle lock;
    uint8_t num;
};

#define SPI_MUTEX_LOCK()    do {} while (xSemaphoreTake(spiMap[spi]->spi->lock, portMAX_DELAY) != pdPASS)
#define SPI_MUTEX_UNLOCK()  xSemaphoreGive(spiMap[spi]->spi->lock)

static spi_t _spi_bus_array[4] = {
    {(volatile spi_dev_t *)(DR_REG_SPI0_BASE), NULL, 0},
    {(volatile spi_dev_t *)(DR_REG_SPI1_BASE), NULL, 1},
    {(volatile spi_dev_t *)(DR_REG_SPI2_BASE), NULL, 2}, // HSPI
    {(volatile spi_dev_t *)(DR_REG_SPI3_BASE), NULL, 3}  // VSPI
};


typedef enum SPI_Num_Def {
    SPI_0 = 0,
    SPI_1 = 1
} SPI_Num_Def;

typedef struct ESP32_SPI_Info {
    uint16_t SPI_SCK_Pin;
    uint16_t SPI_MISO_Pin;
    uint16_t SPI_MOSI_Pin;
    uint16_t SPI_SS_Pin;

    uint32_t SPI_Clock_Div;
    uint8_t SPI_Data_Mode;
    uint8_t SPI_Bit_Order;

    bool SPI_Bit_Order_Set;
    bool SPI_Data_Mode_Set;
    bool SPI_Clock_Divider_Set;

    bool SPI_Enabled;
    bool SPI_USE_HW_SS;

    spi_t *spi;
} ESP32_SPI_Info;

/*
 * SPI mapping
 */
ESP32_SPI_Info SPI_MAP[TOTAL_SPIS] =
{
    // The pin definition can be changed to any pin
    //               VSPI                 HSPI
    // SCK          GPIO18(D2)         GPIO14(A5)
    // MISO         GPIO17(D3)         GPIO12(A6)
    // MOSI         GPIO16(D4)         GPIO13(A8)
    // SS           GPIO5 (D5)         GPIO15(A7)
    {SCK, MISO, MOSI, SS},  // VSPI SCK MISO MOSI SS
    {SCK1, MISO1, MOSI1, SS1}   // HSPI SCK MISO MOSI SS
};

static ESP32_SPI_Info *spiMap[TOTAL_SPIS]; // pointer to SPI_MAP[] containing SPI peripheral register locations (etc)

void HAL_SPI_Initial(HAL_SPI_Interface spi)
{
    /* if(spi > 2){ */
    /*     return; */
    /* } */
    spiMap[spi] = &SPI_MAP[spi];

    spiMap[spi]->SPI_Bit_Order_Set     = false;
    spiMap[spi]->SPI_Data_Mode_Set     = false;
    spiMap[spi]->SPI_Clock_Divider_Set = false;

    spiMap[spi]->SPI_Enabled           = false;
    spiMap[spi]->SPI_USE_HW_SS         = false;
    spiMap[spi]->spi                   = NULL;
}

void HAL_SPI_Begin(HAL_SPI_Interface spi, uint16_t pin)
{
    uint32_t clockDiv = spiFrequencyToClockDiv(1000000);

    if (spi == 0) {
        spiMap[spi]->spi = &_spi_bus_array[3]; // VSPI
    }
    else if (spi == 1) {
        spiMap[spi]->spi = &_spi_bus_array[2]; // HSPI
    }

    if(spiMap[spi]->spi->lock == NULL){
        spiMap[spi]->spi->lock = xSemaphoreCreateMutex();
        if(spiMap[spi]->spi->lock == NULL) {
            return;
        }
    }

    if(spi == 0) {
        SET_PERI_REG_MASK(DPORT_PERIP_CLK_EN_REG, DPORT_SPI_CLK_EN_2);
        CLEAR_PERI_REG_MASK(DPORT_PERIP_RST_EN_REG, DPORT_SPI_RST_2);
        spiMap[spi]->spi->num = 3; // VSPI (3)

    } else if(spi == 1) {
        SET_PERI_REG_MASK(DPORT_PERIP_CLK_EN_REG, DPORT_SPI_CLK_EN_1);
        CLEAR_PERI_REG_MASK(DPORT_PERIP_RST_EN_REG, DPORT_SPI_RST_1);
        spiMap[spi]->spi->num = 2; // HSPI (2)

    } else {
        return;
    }

    spiStopBus(spiMap[spi]->spi);

    if(!spiMap[spi]->SPI_Data_Mode_Set)
        spiSetDataMode(spiMap[spi]->spi, SPI_MODE0);
    else
        spiSetDataMode(spiMap[spi]->spi, spiMap[spi]->SPI_Data_Mode);

    if(!spiMap[spi]->SPI_Bit_Order_Set)
        spiSetBitOrder(spiMap[spi]->spi, SPI_MSBFIRST);
    else
        spiSetBitOrder(spiMap[spi]->spi, spiMap[spi]->SPI_Bit_Order);

    if(!spiMap[spi]->SPI_Clock_Divider_Set)
        spiSetClockDiv(spiMap[spi]->spi, SPI_CLOCK_DIV8);
    else
        spiSetClockDiv(spiMap[spi]->spi, spiMap[spi]->SPI_Clock_Div);

    SPI_MUTEX_LOCK();
    spiMap[spi]->spi->dev->user.usr_mosi = 1;
    spiMap[spi]->spi->dev->user.usr_miso = 1;
    spiMap[spi]->spi->dev->user.doutdin = 1;

    int i;
    for(i=0; i<16; i++) {
        spiMap[spi]->spi->dev->data_buf[i] = 0x00000000;
    }

    SPI_MUTEX_UNLOCK();

    EESP32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    pin_t sck  = PIN_MAP[spiMap[spi]->SPI_SCK_Pin].gpio_pin;
    pin_t miso = PIN_MAP[spiMap[spi]->SPI_MISO_Pin].gpio_pin;
    pin_t mosi = PIN_MAP[spiMap[spi]->SPI_MOSI_Pin].gpio_pin;
    pin_t ss   = PIN_MAP[spiMap[spi]->SPI_SS_Pin].gpio_pin;

    spiAttachSCK(spiMap[spi]->spi, sck);
    spiAttachMISO(spiMap[spi]->spi, miso);
    spiAttachMOSI(spiMap[spi]->spi, mosi);
    spiMap[spi]->SPI_Enabled = true;
}

void HAL_SPI_Begin_Ext(HAL_SPI_Interface spi, SPI_Mode mode, uint16_t pin, void* reserved)
{
    // nothing need to do
}

static void setHwCs(HAL_SPI_Interface spi, bool use)
{
    EESP32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    if(use && !spiMap[spi]->SPI_USE_HW_SS) {
        spiAttachSS(spiMap[spi]->spi, 0, PIN_MAP[spiMap[spi]->SPI_SS_Pin].gpio_pin);
        spiSSEnable(spiMap[spi]->spi);
    } else if(spiMap[spi]->SPI_USE_HW_SS) {
        spiSSDisable(spiMap[spi]->spi);
        spiDetachSS(spiMap[spi]->spi, PIN_MAP[spiMap[spi]->SPI_SS_Pin].gpio_pin);
    }
    spiMap[spi]->SPI_USE_HW_SS = use;
}

void HAL_SPI_End(HAL_SPI_Interface spi)
{
    if(!spiMap[spi]->spi) {
        return;
    }
    EESP32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    spiDetachSCK(spiMap[spi]->spi, PIN_MAP[spiMap[spi]->SPI_SCK_Pin].gpio_pin);
    spiDetachMISO(spiMap[spi]->spi, PIN_MAP[spiMap[spi]->SPI_MISO_Pin].gpio_pin);
    spiDetachMOSI(spiMap[spi]->spi, PIN_MAP[spiMap[spi]->SPI_MOSI_Pin].gpio_pin);
    setHwCs(spi, false);
    spiStopBus(spiMap[spi]->spi);
    spiMap[spi]->spi = NULL;
    spiMap[spi]->SPI_Bit_Order_Set     = false;
    spiMap[spi]->SPI_Data_Mode_Set     = false;
    spiMap[spi]->SPI_Clock_Divider_Set = false;
    spiMap[spi]->SPI_Enabled           = false;
    spiMap[spi]->SPI_USE_HW_SS         = false;
}

void HAL_SPI_Set_Bit_Order(HAL_SPI_Interface spi, uint8_t order)
{
    /* spiSetBitOrder(spiMap[spi]->spi, order); */
    if(!spiMap[spi]->SPI_Bit_Order_Set)
    {
        spiMap[spi]->SPI_Bit_Order_Set = true;
        spiMap[spi]->SPI_Bit_Order = order;
    }
}

void HAL_SPI_Set_Data_Mode(HAL_SPI_Interface spi, uint8_t mode)
{
    if(!spiMap[spi]->SPI_Data_Mode_Set)
    {
        spiMap[spi]->SPI_Data_Mode_Set = true;
        spiMap[spi]->SPI_Data_Mode = mode;
    }
    /* spiSetDataMode(spiMap[spi]->spi, mode); */
}

void HAL_SPI_Set_Clock_Divider(HAL_SPI_Interface spi, uint8_t rate)
{
    if(!spiMap[spi]->SPI_Clock_Divider_Set)
    {
        spiMap[spi]->SPI_Clock_Divider_Set = true;
        switch (rate){
        case 0x00: { // SPI_CLOCK_DIV2
                /* spiSetClockDiv(spiMap[spi]->spi, SPI_CLOCK_DIV2); */
                spiMap[spi]->SPI_Clock_Div = SPI_CLOCK_DIV2;
                break;
            }
        case 0x08: { // SPI_CLOCK_DIV4
            /* spiSetClockDiv(spiMap[spi]->spi, SPI_CLOCK_DIV4); */
            spiMap[spi]->SPI_Clock_Div = SPI_CLOCK_DIV4;
            break;
        }
        case 0x10: {
            /* spiSetClockDiv(spiMap[spi]->spi, SPI_CLOCK_DIV8); */
            spiMap[spi]->SPI_Clock_Div = SPI_CLOCK_DIV8;
            break;
        }
        case 0x18: {
            /* spiSetClockDiv(spiMap[spi]->spi, SPI_CLOCK_DIV16); */
            spiMap[spi]->SPI_Clock_Div = SPI_CLOCK_DIV16;
            break;
        }
        case 0x20: {
            /* spiSetClockDiv(spiMap[spi]->spi, SPI_CLOCK_DIV32); */
            spiMap[spi]->SPI_Clock_Div = SPI_CLOCK_DIV32;
            break;
        }
        case 0x28: {
            /* spiSetClockDiv(spiMap[spi]->spi, SPI_CLOCK_DIV64); */
            spiMap[spi]->SPI_Clock_Div = SPI_CLOCK_DIV64;
            break;
        }
        case 0x30: {
            /* spiSetClockDiv(spiMap[spi]->spi, SPI_CLOCK_DIV128); */
            spiMap[spi]->SPI_Clock_Div = SPI_CLOCK_DIV128;
            break;
        }
        default:
            break;
        }
    }
}

uint16_t HAL_SPI_Send_Receive_Data(HAL_SPI_Interface spi, uint16_t data)
{
    return spiTransferByte(spiMap[spi]->spi,data);
}

bool HAL_SPI_Is_Enabled(HAL_SPI_Interface spi)
{
    return spiMap[spi]->SPI_Enabled;
}

// Don't need below
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
