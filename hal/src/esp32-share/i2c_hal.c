/**
 ******************************************************************************
 * @file    i2c_hal.c
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
#include "i2c_hal.h"
#include "gpio_hal.h"
#include "pinmap_impl.h"

#include "esp32-hal-i2c.h"
#include "esp32-hal.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "rom/ets_sys.h"
#include "soc/i2c_reg.h"
#include "soc/i2c_struct.h"
#include "soc/dport_reg.h"

#define BUFFER_LENGTH   (I2C_BUFFER_LENGTH)

#define I2C_SCL_IDX(p)  ((p==0)?I2CEXT0_SCL_OUT_IDX:((p==1)?I2CEXT1_SCL_OUT_IDX:0))
#define I2C_SDA_IDX(p) ((p==0)?I2CEXT0_SDA_OUT_IDX:((p==1)?I2CEXT1_SDA_OUT_IDX:0))

struct i2c_struct_t {
    i2c_dev_t * dev;
#if !CONFIG_DISABLE_HAL_LOCKS
    xSemaphoreHandle lock;
#endif
    uint8_t num;
};

enum {
    I2C_CMD_RSTART,
    I2C_CMD_WRITE,
    I2C_CMD_READ,
    I2C_CMD_STOP,
    I2C_CMD_END
};

#define I2C_MUTEX_LOCK()    do {} while (xSemaphoreTake(i2cMap[i2c]->i2c->lock, portMAX_DELAY) != pdPASS)
#define I2C_MUTEX_UNLOCK()  xSemaphoreGive(i2cMap[i2c]->i2c->lock)

static i2c_t _i2c_bus_array[2] = {
    {(volatile i2c_dev_t *)(DR_REG_I2C_EXT_BASE), NULL, 0},
    {(volatile i2c_dev_t *)(DR_REG_I2C1_EXT_BASE), NULL, 1}
};

// default pin
// I2C SDA D0 GPIO21
// I2C SCL D1 GPIO22
// The other is define by self which can be changed
// We define below
typedef enum I2C_Num_Def {
    I2C_0 = 0,
    I2C_1 = 1
} I2C_Num_Def;

typedef struct ESP32_I2C_Info {
    uint8_t I2C_SCL_Pin;
    uint8_t I2C_SDA_Pin;

    bool I2C_Enabled;
    i2c_t* i2c;
    uint8_t rxBuffer[BUFFER_LENGTH];
    uint8_t rxBufferIndex;
    uint8_t rxBufferLength;
    uint8_t txBuffer[BUFFER_LENGTH];
    uint8_t txBufferIndex;
    uint8_t txBufferLength;
    uint8_t txAddress;
    uint8_t transmitting;
    void (*callback_onRequest)(void);
    void (*callback_onReceive)(int);
} ESP32_I2C_Info;

/*
 * I2C mapping
 */
ESP32_I2C_Info I2C_MAP[TOTAL_I2CS] =
{
        { SCL, SDA},    // I2C0  D0, D1
        { SCL1, SDA1}   // I2C1  A3, A4
};

/* static ESP32_I2C_Info *i2cMap[TOTAL_I2CS]; // pointer to I2C_MAP[] containing I2C peripheral register locations (etc) */
ESP32_I2C_Info *i2cMap[TOTAL_I2CS]; // pointer to I2C_MAP[] containing I2C peripheral register locations (etc)

void HAL_I2C_Initial(HAL_I2C_Interface i2c, void* reserved)
{
    i2cMap[i2c] = &I2C_MAP[i2c];

    i2cMap[i2c]->I2C_Enabled = false;
    i2cMap[i2c]->transmitting = 0;
}

void HAL_I2C_Set_Speed(HAL_I2C_Interface i2c, uint32_t speed, void* reserved)
{
    uint32_t period = (APB_CLK_FREQ/speed) / 2;

    if(i2cMap[i2c]->i2c == NULL){
        return;
    }

    I2C_MUTEX_LOCK();
    i2cMap[i2c]->i2c->dev->scl_low_period.period = period;
    i2cMap[i2c]->i2c->dev->scl_high_period.period = period;

    i2cMap[i2c]->i2c->dev->scl_start_hold.time = 50;
    i2cMap[i2c]->i2c->dev->scl_rstart_setup.time = 50;

    i2cMap[i2c]->i2c->dev->scl_stop_hold.time   = 50;
    i2cMap[i2c]->i2c->dev->scl_stop_setup.time = 50;

    i2cMap[i2c]->i2c->dev->sda_hold.time     = 25;
    i2cMap[i2c]->i2c->dev->sda_sample.time = 25;
    I2C_MUTEX_UNLOCK();
}

void HAL_I2C_Stretch_Clock(HAL_I2C_Interface i2c, bool stretch, void* reserved)
{

}

void HAL_I2C_Begin(HAL_I2C_Interface i2c, I2C_Mode mode, uint8_t address, void* reserved)
{
    i2cMap[i2c]->i2c = i2cInit(i2c, 0, false);

    i2cMap[i2c]->i2c->num = i2c;

    i2cSetFrequency(i2cMap[i2c]->i2c, 100000);

    EESP32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    pin_t scl_pin = PIN_MAP[i2cMap[i2c]->I2C_SCL_Pin].gpio_pin;
    pin_t sda_pin = PIN_MAP[i2cMap[i2c]->I2C_SDA_Pin].gpio_pin;

    i2cAttachSCL(i2cMap[i2c]->i2c, scl_pin);
    i2cAttachSDA(i2cMap[i2c]->i2c, sda_pin);
    HAL_I2C_Flush_Data(i2c, NULL);
    i2cInitFix(i2cMap[i2c]->i2c);
}

void HAL_I2C_End(HAL_I2C_Interface i2c,void* reserved)
{
    // xxx: To be done
}

uint32_t HAL_I2C_Request_Data(HAL_I2C_Interface i2c, uint8_t address, uint8_t quantity, uint8_t stop,void* reserved)
{
    if (quantity > BUFFER_LENGTH) {
        quantity = BUFFER_LENGTH;
    }
    size_t receiveLength = i2cRead(i2cMap[i2c]->i2c, address, false, i2cMap[i2c]->rxBuffer, quantity, stop) == 0? quantity: 0;
    i2cMap[i2c]->rxBufferIndex = 0;
    i2cMap[i2c]->rxBufferLength = receiveLength;
    return receiveLength;
}

void HAL_I2C_Begin_Transmission(HAL_I2C_Interface i2c, uint8_t address,void* reserved)
{
    i2cMap[i2c]->transmitting = 1;
    i2cMap[i2c]->txAddress = address;
    i2cMap[i2c]->txBufferIndex = 0;
    i2cMap[i2c]->txBufferLength = 0;
}

uint8_t HAL_I2C_End_Transmission(HAL_I2C_Interface i2c, uint8_t stop,void* reserved)
{
    int8_t ret = i2cWrite(i2cMap[i2c]->i2c, i2cMap[i2c]->txAddress, false, i2cMap[i2c]->txBuffer, i2cMap[i2c]->txBufferLength, stop);
    i2cMap[i2c]->txBufferIndex = 0;
    i2cMap[i2c]->txBufferLength = 0;
    i2cMap[i2c]->transmitting = 0;
    return 0;
}

uint32_t HAL_I2C_Write_Data(HAL_I2C_Interface i2c, uint8_t data,void* reserved)
{
    if (i2cMap[i2c]->transmitting) {
        if (i2cMap[i2c]->txBufferLength >= BUFFER_LENGTH) {
            return 0;
        }
        i2cMap[i2c]->txBuffer[i2cMap[i2c]->txBufferIndex++] = data;
        i2cMap[i2c]->txBufferLength = i2cMap[i2c]->txBufferIndex;
        return 0;
    }
    return 1;
}

int32_t HAL_I2C_Available_Data(HAL_I2C_Interface i2c,void* reserved)
{
    return (i2cMap[i2c]->rxBufferLength - i2cMap[i2c]->rxBufferIndex);
}

int32_t HAL_I2C_Read_Data(HAL_I2C_Interface i2c,void* reserved)
{
    int value = -1;
    if(i2cMap[i2c]->rxBufferIndex < i2cMap[i2c]->rxBufferLength){
        value = i2cMap[i2c]->rxBuffer[i2cMap[i2c]->rxBufferIndex++];
    }
    return value;
}

int32_t HAL_I2C_Peek_Data(HAL_I2C_Interface i2c,void* reserved)
{
    int value = -1;
    if(i2cMap[i2c]->rxBufferIndex < i2cMap[i2c]->rxBufferLength){
        value = i2cMap[i2c]->rxBuffer[i2cMap[i2c]->rxBufferIndex];
    }
    return value;
}

void HAL_I2C_Flush_Data(HAL_I2C_Interface i2c,void* reserved)
{
    i2cMap[i2c]->rxBufferIndex = 0;
    i2cMap[i2c]->rxBufferLength = 0;
    i2cMap[i2c]->txBufferIndex = 0;
    i2cMap[i2c]->txBufferLength = 0;
}

bool HAL_I2C_Is_Enabled(HAL_I2C_Interface i2c,void* reserved)
{
    return i2cMap[i2c]->I2C_Enabled;
}

void HAL_I2C_Set_Callback_On_Receive(HAL_I2C_Interface i2c, void (*function)(int),void* reserved)
{
    i2cMap[i2c]->callback_onReceive = function;
}

void HAL_I2C_Set_Callback_On_Request(HAL_I2C_Interface i2c, void (*function)(void),void* reserved)
{
    i2cMap[i2c]->callback_onRequest = function;
}

/*******************************************************************************
 * Function Name  : HAL_I2C1_EV_Handler (Declared as weak in stm32_it.cpp)
 * Description    : This function handles I2C1 Event interrupt request(Only for Slave mode).
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void HAL_I2C1_EV_Handler(void)
{
}

/*******************************************************************************
 * Function Name  : HAL_I2C1_ER_Handler (Declared as weak in stm32_it.cpp)
 * Description    : This function handles I2C1 Error interrupt request.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void HAL_I2C1_ER_Handler(void)
{
}

void HAL_I2C_Enable_DMA_Mode(HAL_I2C_Interface i2c, bool enable,void* reserved)
{
}
