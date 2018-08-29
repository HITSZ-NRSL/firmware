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
#include "hw_config.h"
#include "pinmap_impl.h"

const static char *TAG = "hal";

unsigned char twi_dcount = 18;
static unsigned char twi_sda, twi_scl;
static uint32_t twi_clockStretchLimit;

#define SDA_LOW()   (GPES = (1 << twi_sda)) //Enable SDA (becomes output and since GPO is 0 for the pin, it will pull the line low)
#define SDA_HIGH()  (GPEC = (1 << twi_sda)) //Disable SDA (becomes input and since it has pullup it will go high)
#define SDA_READ()  ((GPI & (1 << twi_sda)) != 0)
#define SCL_LOW()   (GPES = (1 << twi_scl))
#define SCL_HIGH()  (GPEC = (1 << twi_scl))
#define SCL_READ()  ((GPI & (1 << twi_scl)) != 0)

#ifndef FCPU80
#define FCPU80 80000000L
#endif

#if F_CPU == FCPU80
#define TWI_CLOCK_STRETCH_MULTIPLIER 3
#else
#define TWI_CLOCK_STRETCH_MULTIPLIER 6
#endif

#define BUFFER_LENGTH 32
static uint8_t rxBufferIndex = 0;
static uint8_t rxBufferLength = 0;
static uint8_t txBufferIndex = 0;
static uint8_t txBufferLength = 0;
static uint8_t transmitting = 0;
uint8_t txAddress = 0;

uint8_t rxBuffer[BUFFER_LENGTH];
uint8_t txBuffer[BUFFER_LENGTH];

static int default_sda_pin = SDA;
static int default_scl_pin = SCL;

void twi_setClockStretchLimit(uint32_t limit){
    twi_clockStretchLimit = limit * TWI_CLOCK_STRETCH_MULTIPLIER;
}

void twi_setClock(unsigned int freq){
#if F_CPU == FCPU80
    if(freq <= 100000) twi_dcount = 19;//about 100KHz
    else if(freq <= 200000) twi_dcount = 8;//about 200KHz
    else if(freq <= 300000) twi_dcount = 3;//about 300KHz
    else if(freq <= 400000) twi_dcount = 1;//about 400KHz
    else twi_dcount = 1;//about 400KHz
#else
    if(freq <= 100000) twi_dcount = 32;//about 100KHz
    else if(freq <= 200000) twi_dcount = 14;//about 200KHz
    else if(freq <= 300000) twi_dcount = 8;//about 300KHz
    else if(freq <= 400000) twi_dcount = 5;//about 400KHz
    else if(freq <= 500000) twi_dcount = 3;//about 500KHz
    else if(freq <= 600000) twi_dcount = 2;//about 600KHz
    else twi_dcount = 1;//about 700KHz
#endif
}
void twi_init(unsigned char sda, unsigned char scl){
    twi_sda = sda;
    twi_scl = scl;
    /* __pinMode(twi_sda, ESP8266_INPUT_PULLUP); */
    GPF(twi_sda) = GPFFS(GPFFS_GPIO(twi_sda));//Set mode to GPIO
    GPEC = (1 << twi_sda); //Disable
    GPC(twi_sda) = (GPC(twi_sda) & (0xF << GPCI)) | (1 << GPCD); //SOURCE(GPIO) | DRIVER(OPEN_DRAIN) | INT_TYPE(UNCHANGED) | WAKEUP_ENABLE(DISABLED)
    GPF(twi_sda) |= (1 << GPFPU);  // Enable  Pullup

    /* __pinMode(twi_scl, ESP8266_INPUT_PULLUP); */
    GPF(twi_scl) = GPFFS(GPFFS_GPIO(twi_scl));//Set mode to GPIO
    GPEC = (1 << twi_scl); //Disable
    GPC(twi_scl) = (GPC(twi_scl) & (0xF << GPCI)) | (1 << GPCD); //SOURCE(GPIO) | DRIVER(OPEN_DRAIN) | INT_TYPE(UNCHANGED) | WAKEUP_ENABLE(DISABLED)
    GPF(twi_scl) |= (1 << GPFPU);  // Enable  Pullup

    twi_setClock(100000);
    twi_setClockStretchLimit(230); // default value is 230 uS
}

void twi_stop(void){
    /* __pinMode(twi_sda, ESP8266_INPUT); */
    GPF(twi_sda) = GPFFS(GPFFS_GPIO(twi_sda));//Set mode to GPIO
    GPEC = (1 << twi_sda); //Disable
    GPC(twi_sda) = (GPC(twi_sda) & (0xF << GPCI)) | (1 << GPCD); //SOURCE(GPIO) | DRIVER(OPEN_DRAIN) | INT_TYPE(UNCHANGED) | WAKEUP_ENABLE(DISABLED)

    /* __pinMode(twi_scl, ESP8266_INPUT); */
    GPF(twi_scl) = GPFFS(GPFFS_GPIO(twi_scl));//Set mode to GPIO
    GPEC = (1 << twi_scl); //Disable
    GPC(twi_scl) = (GPC(twi_scl) & (0xF << GPCI)) | (1 << GPCD); //SOURCE(GPIO) | DRIVER(OPEN_DRAIN) | INT_TYPE(UNCHANGED) | WAKEUP_ENABLE(DISABLED)
}

static void twi_delay(unsigned char v){
    unsigned int i;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
    unsigned int reg;
    for(i=0;i<v;i++) reg = GPI;
#pragma GCC diagnostic pop
}

static bool twi_write_start(void) {
    SCL_HIGH();
    SDA_HIGH();
    if (SDA_READ() == 0) return false;
    twi_delay(twi_dcount);
    SDA_LOW();
    twi_delay(twi_dcount);
    return true;
}

static bool twi_write_stop(void){
    uint32_t i = 0;
    SCL_LOW();
    SDA_LOW();
    twi_delay(twi_dcount);
    SCL_HIGH();
    while (SCL_READ() == 0 && (i++) < twi_clockStretchLimit); // Clock stretching
    twi_delay(twi_dcount);
    SDA_HIGH();
    twi_delay(twi_dcount);
    return true;
}


static bool twi_write_bit(bool bit) {
    uint32_t i = 0;
    SCL_LOW();
    if (bit) SDA_HIGH();
    else SDA_LOW();
    twi_delay(twi_dcount+1);
    SCL_HIGH();
    while (SCL_READ() == 0 && (i++) < twi_clockStretchLimit);// Clock stretching
    twi_delay(twi_dcount);
    return true;
}

static bool twi_read_bit(void) {
    uint32_t i = 0;
    SCL_LOW();
    SDA_HIGH();
    twi_delay(twi_dcount+2);
    SCL_HIGH();
    while (SCL_READ() == 0 && (i++) < twi_clockStretchLimit);// Clock stretching
    bool bit = SDA_READ();
    twi_delay(twi_dcount);
    return bit;
}

static bool twi_write_byte(unsigned char byte) {
    unsigned char bit;
    for (bit = 0; bit < 8; bit++) {
        twi_write_bit(byte & 0x80);
        byte <<= 1;
    }
    return !twi_read_bit();//NACK/ACK
}

static unsigned char twi_read_byte(bool nack) {
    unsigned char byte = 0;
    unsigned char bit;
    for (bit = 0; bit < 8; bit++) byte = (byte << 1) | twi_read_bit();
    twi_write_bit(nack);
    return byte;
}

unsigned char twi_writeTo(unsigned char address, unsigned char * buf, unsigned int len, unsigned char sendStop){
    unsigned int i;
    if(!twi_write_start()) return 4;//line busy
    if(!twi_write_byte(((address << 1) | 0) & 0xFF)) {
        if (sendStop) twi_write_stop();
        return 2; //received NACK on transmit of address
    }
    for(i=0; i<len; i++) {
        if(!twi_write_byte(buf[i])) {
            if (sendStop) twi_write_stop();
            return 3;//received NACK on transmit of data
        }
    }
    if(sendStop) twi_write_stop();
    i = 0;
    while(SDA_READ() == 0 && (i++) < 10){
        SCL_LOW();
        twi_delay(twi_dcount);
        SCL_HIGH();
        twi_delay(twi_dcount);
    }
    return 0;
}

unsigned char twi_readFrom(unsigned char address, unsigned char* buf, unsigned int len, unsigned char sendStop){
    unsigned int i;
    if(!twi_write_start()) return 4;//line busy
    if(!twi_write_byte(((address << 1) | 1) & 0xFF)) {
        if (sendStop) twi_write_stop();
        return 2;//received NACK on transmit of address
    }
    for(i=0; i<(len-1); i++) buf[i] = twi_read_byte(false);
    buf[len-1] = twi_read_byte(true);
    if(sendStop) twi_write_stop();
    i = 0;
    while(SDA_READ() == 0 && (i++) < 10){
        SCL_LOW();
        twi_delay(twi_dcount);
        SCL_HIGH();
        twi_delay(twi_dcount);
    }
    return 0;
}


void HAL_I2C_Initial(HAL_I2C_Interface i2c, void* reserved)
{
    //MOLMC_LOGD(TAG, "Enter HAL_I2C_Initial...");
}

void HAL_I2C_Set_Speed(HAL_I2C_Interface i2c, uint32_t speed, void* reserved)
{
    twi_setClock(speed);
}

void HAL_I2C_Stretch_Clock(HAL_I2C_Interface i2c, bool stretch, void* reserved)
{
}

void HAL_I2C_Begin(HAL_I2C_Interface i2c, I2C_Mode mode, uint8_t address, void* reserved)
{
    //MOLMC_LOGD(TAG, "Enter HAL_I2C_Begin...");
    // only for default pin
    //if (i2c != HAL_I2C_INTERFACE1)
    EESP8266_Pin_Info* PIN_MAP = HAL_Pin_Map();
    unsigned char sda = PIN_MAP[default_sda_pin].gpio_pin;
    unsigned char scl = PIN_MAP[default_scl_pin].gpio_pin;

    twi_init(sda, scl);
    HAL_I2C_Flush_Data(1, NULL);
}

void HAL_I2C_End(HAL_I2C_Interface i2c,void* reserved)
{
    twi_stop();
}
uint32_t HAL_I2C_Request_Data(HAL_I2C_Interface i2c, uint8_t address, uint8_t quantity, uint8_t stop,void* reserved)
{
    //MOLMC_LOGD(TAG, "Enter HAL_I2C_Request_Data...");
    if(quantity > BUFFER_LENGTH){
        quantity = BUFFER_LENGTH;
    }
    uint32_t read = (twi_readFrom(address, rxBuffer, quantity, stop) == 0)?quantity:0;
    //MOLMC_LOGD(TAG, "read: %d", read);
    rxBufferIndex = 0;
    rxBufferLength = read;
    return read;
}

void HAL_I2C_Begin_Transmission(HAL_I2C_Interface i2c, uint8_t address,void* reserved)
{
    transmitting = 1;
    txAddress = address;
    txBufferIndex = 0;
    txBufferLength = 0;

}

uint8_t HAL_I2C_End_Transmission(HAL_I2C_Interface i2c, uint8_t stop,void* reserved)
{
    int8_t ret = twi_writeTo(txAddress, txBuffer, txBufferLength, stop);
    txBufferIndex = 0;
    txBufferLength = 0;
    transmitting = 0;
    return ret;
}

uint32_t HAL_I2C_Write_Data(HAL_I2C_Interface i2c, uint8_t data,void* reserved)
{
    //MOLMC_LOGD(TAG, "Enter HAL_I2C_Write_Data...");
    //MOLMC_LOGD(TAG, "data: %d", data);
    if(transmitting){
        if(txBufferLength >= BUFFER_LENGTH){
            //setWriteError();
            //MOLMC_LOGD(TAG, "HAL_I2C_Write_Data buffer length error");
            return 0;
        }
        txBuffer[txBufferIndex] = data;
        ++txBufferIndex;
        txBufferLength = txBufferIndex;
    } else {
        // i2c_slave_transmit(&data, 1);
    }
    return 1;
}

int32_t HAL_I2C_Available_Data(HAL_I2C_Interface i2c,void* reserved)
{
    int result = rxBufferLength - rxBufferIndex;

    if (!result) {
        // yielding here will not make more data "available",
        // but it will prevent the system from going into WDT reset
        optimistic_yield(1000);
    }

    return result;
}

int32_t HAL_I2C_Read_Data(HAL_I2C_Interface i2c,void* reserved)
{
    int value = -1;
    if(rxBufferIndex < rxBufferLength){
        value = rxBuffer[rxBufferIndex];
        ++rxBufferIndex;
    }
    return value;
}

int32_t HAL_I2C_Peek_Data(HAL_I2C_Interface i2c,void* reserved)
{
    int value = -1;
    if(rxBufferIndex < rxBufferLength){
        value = rxBuffer[rxBufferIndex];
    }
    return value;
}

void HAL_I2C_Flush_Data(HAL_I2C_Interface i2c,void* reserved)
{
    rxBufferIndex = 0;
    rxBufferLength = 0;
    txBufferIndex = 0;
    txBufferLength = 0;

    // XXX: to be implemented.
}

bool HAL_I2C_Is_Enabled(HAL_I2C_Interface i2c,void* reserved)
{
    return false;
}

void HAL_I2C_Set_Callback_On_Receive(HAL_I2C_Interface i2c, void (*function)(int),void* reserved)
{

}

void HAL_I2C_Set_Callback_On_Request(HAL_I2C_Interface i2c, void (*function)(void),void* reserved)
{

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
