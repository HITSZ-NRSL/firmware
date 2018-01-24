/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech

Description: SX1276 driver specific target board functions implementation

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis and Gregory Cristian
*/

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "radio.h"
#include "sx1276.h"
#include "sx1276-board.h"
#include "timer.h"
#include "rtc_hal.h"

#include "wiring.h"
#include "wiring_interrupts.h"
#include "wiring_spi.h"
#include "wiring_system.h"


/*!
 * Flag used to set the RF switch control pins in low power mode when the radio is not active.
 */
static bool RadioIsActive = false;

static bool rtcSetCallback = false;
/*!
 * Radio driver structure initialization
 */
const struct Radio_s Radio =
{
    SX1276Init,
    SX1276GetStatus,
    SX1276SetModem,
    SX1276SetChannel,
    SX1276IsChannelFree,
    SX1276Random,
    SX1276SetRxConfig,
    SX1276SetTxConfig,
    SX1276CheckRfFrequency,
    SX1276GetTimeOnAir,
    SX1276Send,
    SX1276SetSleep,
    SX1276SetStby,
    SX1276SetRx,
    SX1276StartCad,
    SX1276SetTxContinuousWave,
    SX1276ReadRssi,
    SX1276Write,
    SX1276Read,
    SX1276WriteBuffer,
    SX1276ReadBuffer,
    SX1276SetMaxPayloadLength,
    SX1276SetPublicNetwork
};

void SX1276IoInit( void )
{
    //定义1278引脚
    SX1276.Reset  = SX1278_RESET;
    SX1276.DIO0   = SX1278_DIO0;
    SX1276.DIO1   = SX1278_DIO1;
    SX1276.DIO2   = SX1278_DIO2;
    SX1276.DIO3   = SX1278_DIO3;
    SX1276.DIO4   = SX1278_DIO4;
    SX1276.DIO5   = SX1278_DIO5;
    SX1276.SpiNss = SX1278_NSS;
    SX1276.RxTx   = SX1278_RXTX;

    pinMode(SX1276.SpiNss,OUTPUT);
    digitalWrite(SX1276.SpiNss,1);
    pinMode(SX1276.DIO0,INPUT_PULLUP);
    pinMode(SX1276.DIO1,INPUT_PULLUP);
    pinMode(SX1276.DIO2,INPUT_PULLUP);
    pinMode(SX1276.DIO3,INPUT_PULLUP);
    pinMode(SX1276.DIO4,INPUT_PULLUP);
    pinMode(SX1276.DIO5,INPUT_PULLUP);
    pinMode(SX1278_BATTERY_POWER,AN_INPUT);
}

void SX1276IoDeInit( void )
{
    pinMode(SX1276.SpiNss,OUTPUT);
    digitalWrite(SX1276.SpiNss,1);
    pinMode(SX1276.DIO0,INPUT);
    pinMode(SX1276.DIO1,INPUT);
    pinMode(SX1276.DIO2,INPUT);
    pinMode(SX1276.DIO3,INPUT);
    pinMode(SX1276.DIO4,INPUT);
    pinMode(SX1276.DIO5,INPUT);
}

void SX1276IoIrqInit( DioIrqHandler **irqHandlers )
{
    attachInterrupt(SX1276.DIO0, irqHandlers[0], RISING, 1, 0);
    attachInterrupt(SX1276.DIO1, irqHandlers[1], RISING, 1, 0);
    attachInterrupt(SX1276.DIO2, irqHandlers[2], RISING, 1, 0);
    attachInterrupt(SX1276.DIO3, irqHandlers[3], RISING, 1, 0);
    attachInterrupt(SX1276.DIO4, irqHandlers[4], RISING, 1, 0);
    // attachInterrupt(SX1276.DIO5, irqHandlers[5], RISING, 1, 0); //DIO5不使用
}

void SX1276IoIrqDeInit( void )
{
    detachInterrupt(SX1276.DIO0);
    detachInterrupt(SX1276.DIO1);
    detachInterrupt(SX1276.DIO2);
    detachInterrupt(SX1276.DIO3);
    detachInterrupt(SX1276.DIO4);
    detachInterrupt(SX1276.DIO5);
}

void SX1276SetRfTxPower( int8_t power )
{
    uint8_t paConfig = 0;
    uint8_t paDac = 0;

    paConfig = SX1276Read( REG_PACONFIG );
    paDac = SX1276Read( REG_PADAC );

    paConfig = ( paConfig & RF_PACONFIG_PASELECT_MASK ) | SX1276GetPaSelect( SX1276.Settings.Channel );
    paConfig = ( paConfig & RF_PACONFIG_MAX_POWER_MASK ) | 0x70;

    if( ( paConfig & RF_PACONFIG_PASELECT_PABOOST ) == RF_PACONFIG_PASELECT_PABOOST )
    {
        if( power > 17 )
        {
            paDac = ( paDac & RF_PADAC_20DBM_MASK ) | RF_PADAC_20DBM_ON;
        }
        else
        {
            paDac = ( paDac & RF_PADAC_20DBM_MASK ) | RF_PADAC_20DBM_OFF;
        }
        if( ( paDac & RF_PADAC_20DBM_ON ) == RF_PADAC_20DBM_ON )
        {
            if( power < 5 )
            {
                power = 5;
            }
            if( power > 20 )
            {
                power = 20;
            }
            paConfig = ( paConfig & RF_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power - 5 ) & 0x0F );
        }
        else
        {
            if( power < 2 )
            {
                power = 2;
            }
            if( power > 17 )
            {
                power = 17;
            }
            paConfig = ( paConfig & RF_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power - 2 ) & 0x0F );
        }
    }
    else
    {
        if( power < -1 )
        {
            power = -1;
        }
        if( power > 14 )
        {
            power = 14;
        }
        paConfig = ( paConfig & RF_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power + 1 ) & 0x0F );
    }
    SX1276Write( REG_PACONFIG, paConfig );
    SX1276Write( REG_PADAC, paDac );
}

uint8_t SX1276GetPaSelect( uint32_t channel )
{
    if( channel < RF_MID_BAND_THRESH )
    {
        return RF_PACONFIG_PASELECT_PABOOST;
    }
    else
    {
        return RF_PACONFIG_PASELECT_RFO;
    }
}

void SX1276SetAntSwLowPower( bool status )
{
    if( RadioIsActive != status )
    {
        RadioIsActive = status;

        if( status == false )
        {
            SX1276AntSwInit( );
        }
        else
        {
            SX1276AntSwDeInit( );
        }
    }
}

void SX1276AntSwInit( void )
{
    //高收低发
    pinMode(SX1276.RxTx,OUTPUT);
    digitalWrite(SX1276.RxTx,1);
}

void SX1276AntSwDeInit( void )
{
    pinMode(SX1276.RxTx,OUTPUT);
    digitalWrite(SX1276.RxTx,0);
}

void SX1276SetAntSw( uint8_t opMode )
{
    switch( opMode )
    {
        case RFLR_OPMODE_TRANSMITTER:
            digitalWrite(SX1276.RxTx,0);
            break;
        case RFLR_OPMODE_RECEIVER:
        case RFLR_OPMODE_RECEIVER_SINGLE:
        case RFLR_OPMODE_CAD:
        default:
            digitalWrite(SX1276.RxTx,1);
            break;
    }
}

bool SX1276CheckRfFrequency( uint32_t frequency )
{
    // Implement check. Currently all frequencies are supported
    return true;
}

void SX1276BoardInit(void)
{
    SPI1.setBitOrder(MSBFIRST);
    SPI1.setClockDivider(SPI_CLOCK_DIV16);
    SPI1.setDataMode(0);
    SPI1.begin();
    SX1276IoInit();
    if(!rtcSetCallback){
        rtcSetCallback = true;
        HAL_RTC_SetCallbacks(TimerIrqHandler, NULL);
    }
}

void SX1276SetReset(void)
{
    pinMode(SX1276.Reset,OUTPUT);
    digitalWrite(SX1276.Reset,0);
    DelayMs(1);
    pinMode(SX1276.Reset,INPUT);
    DelayMs(6);
}

void SX1276SpiSetNss(uint8_t val)
{
    digitalWrite(SX1276.SpiNss,val);
}

uint8_t SX1276SpiTransfer( uint8_t outData )
{
    SPI1.transfer(outData);
}
