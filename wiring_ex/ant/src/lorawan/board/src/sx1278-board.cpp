/*
  / _____)             _              | |
  ( (____  _____ ____ _| |_ _____  ____| |__
  \____ \| ___ |    (_   _) ___ |/ ___)  _ \
  _____) ) ____| | | || |_| ____( (___| | | |
  (______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2013 Semtech

  Description: SX1278 driver specific target board functions implementation

  License: Revised BSD License, see LICENSE.TXT file include in the project

  Maintainer: Miguel Luis and Gregory Cristian
*/

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "radio.h"
// #include "sx1278.h"
#include "sx1278-board.h"
#include "wiring.h"
#include "wiring_interrupts.h"
#include "wiring_spi.h"

#define         ID1                                 ( 0x1FF80050 )
#define         ID2                                 ( 0x1FF80054 )
#define         ID3                                 ( 0x1FF80064 )


/*!
 * Flag used to set the RF switch control pins in low power mode when the radio is not active.
 */
static bool RadioIsActive = false;

/*!
 * Radio driver structure initialization
 */
const struct Radio_s Radio =
    {
        SX1278Init,
        SX1278GetStatus,
        SX1278SetModem,
        SX1278SetChannel,
        SX1278IsChannelFree,
        SX1278Random,
        SX1278SetRxConfig,
        SX1278SetTxConfig,
        SX1278CheckRfFrequency,
        SX1278GetTimeOnAir,
        SX1278Send,
        SX1278SetSleep,
        SX1278SetStby,
        SX1278SetRx,
        SX1278StartCad,
        SX1278ReadRssi,
        SX1278Write,
        SX1278Read,
        SX1278WriteBuffer,
        SX1278ReadBuffer,
        SX1278SetMaxPayloadLength
    };

/*!
 * Antenna switch GPIO pins objects
 */
//Gpio_t AntSwitchLf;
//Gpio_t AntSwitchHf;

void SX1278IoInit( void )
{
    /*  GpioInit( &SX1278.Spi.Nss, RADIO_NSS, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 1 );
        GpioInit( &SX1278.DIO0, RADIO_DIO_0, PIN_INPUT, PIN_PUSH_PULL, PIN_PULL_UP, 0 );
        GpioInit( &SX1278.DIO1, RADIO_DIO_1, PIN_INPUT, PIN_PUSH_PULL, PIN_PULL_UP, 0 );
        GpioInit( &SX1278.DIO2, RADIO_DIO_2, PIN_INPUT, PIN_PUSH_PULL, PIN_PULL_UP, 0 );
        GpioInit( &SX1278.DIO3, RADIO_DIO_3, PIN_INPUT, PIN_PUSH_PULL, PIN_PULL_UP, 0 );
        GpioInit( &SX1278.DIO4, RADIO_DIO_4, PIN_INPUT, PIN_PUSH_PULL, PIN_PULL_UP, 0 );
        GpioInit( &SX1278.DIO5, RADIO_DIO_5, PIN_INPUT, PIN_PUSH_PULL, PIN_PULL_UP, 0 );
    */
    pinMode(NSS_SPI1,OUTPUT);
    digitalWrite(NSS_SPI1,1);
    pinMode(DIO0,INPUT_PULLUP);
    pinMode(DIO1,INPUT_PULLUP);
    pinMode(DIO2,INPUT_PULLUP);
    pinMode(DIO3,INPUT_PULLUP);
    pinMode(DIO4,INPUT_PULLUP);
    pinMode(DIO5,INPUT_PULLUP);

    // pinMode(DIO0,INPUT);
    // pinMode(DIO1,INPUT);
    // pinMode(DIO2,INPUT);
    // pinMode(DIO3,INPUT);
    // pinMode(DIO4,INPUT);
    // pinMode(DIO5,INPUT);
}


void SX1278IoIrqInit( DioIrqHandler **irqHandlers )
{
    /*
      GpioSetInterrupt( &SX1278.DIO0, IRQ_RISING_EDGE, IRQ_HIGH_PRIORITY, irqHandlers[0] );
      GpioSetInterrupt( &SX1278.DIO1, IRQ_RISING_EDGE, IRQ_HIGH_PRIORITY, irqHandlers[1] );
      GpioSetInterrupt( &SX1278.DIO2, IRQ_RISING_EDGE, IRQ_HIGH_PRIORITY, irqHandlers[2] );
      GpioSetInterrupt( &SX1278.DIO3, IRQ_RISING_EDGE, IRQ_HIGH_PRIORITY, irqHandlers[3] );
      GpioSetInterrupt( &SX1278.DIO4, IRQ_RISING_EDGE, IRQ_HIGH_PRIORITY, irqHandlers[4] );
      GpioSetInterrupt( &SX1278.DIO5, IRQ_RISING_EDGE, IRQ_HIGH_PRIORITY, irqHandlers[5] );
    */
    attachInterrupt(DIO0, irqHandlers[0], RISING, 1, 0);
    attachInterrupt(DIO1, irqHandlers[1], RISING, 1, 0);
    attachInterrupt(DIO2, irqHandlers[2], RISING, 1, 0);
    attachInterrupt(DIO3, irqHandlers[3], RISING, 1, 0);
    attachInterrupt(DIO4, irqHandlers[4], RISING, 1, 0);
    attachInterrupt(DIO5, irqHandlers[5], RISING, 1, 0);
}

void SX1278IoDeInit( void )
{
    /*
      GpioInit( &SX1278.Spi.Nss, RADIO_NSS, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 1 );

      GpioInit( &SX1278.DIO0, RADIO_DIO_0, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
      GpioInit( &SX1278.DIO1, RADIO_DIO_1, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
      GpioInit( &SX1278.DIO2, RADIO_DIO_2, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
      GpioInit( &SX1278.DIO3, RADIO_DIO_3, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
      GpioInit( &SX1278.DIO4, RADIO_DIO_4, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
      GpioInit( &SX1278.DIO5, RADIO_DIO_5, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
    */
    pinMode(NSS_SPI1,OUTPUT);
    digitalWrite(NSS_SPI1,1);
    pinMode(DIO0,INPUT);
    pinMode(DIO1,INPUT);
    pinMode(DIO2,INPUT);
    pinMode(DIO3,INPUT);
    pinMode(DIO4,INPUT);
    pinMode(DIO5,INPUT);
}


uint8_t SX1278GetPaSelect( uint32_t channel )
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

void SX1278SetAntSwLowPower( bool status )
{

    if( RadioIsActive != status )
        {
            RadioIsActive = status;

            if( status == false )
                {
                    SX1278AntSwInit( );
                }
            else
                {
                    SX1278AntSwDeInit( );
                }
        }

}

void SX1278AntSwInit( void )
{
    /*
      GpioInit( &AntSwitchLf, RADIO_ANT_SWITCH_LF, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 1 );
      GpioInit( &AntSwitchHf, RADIO_ANT_SWITCH_HF, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 0 );
    */
    pinMode(RX_EN,OUTPUT);
    digitalWrite(RX_EN,1);
    pinMode(TX_EN,OUTPUT);
    digitalWrite(TX_EN,0);
}

void SX1278AntSwDeInit( void )
{
    /*
      GpioInit( &AntSwitchLf, RADIO_ANT_SWITCH_LF, PIN_OUTPUT, PIN_OPEN_DRAIN, PIN_NO_PULL, 0 );
      GpioInit( &AntSwitchHf, RADIO_ANT_SWITCH_HF, PIN_OUTPUT, PIN_OPEN_DRAIN, PIN_NO_PULL, 0 );
    */
    pinMode(RX_EN,OUTPUT);
    digitalWrite(RX_EN,0);
    pinMode(TX_EN,OUTPUT);
    digitalWrite(TX_EN,0);
}

void SX1278SetAntSw( uint8_t rxTx )
{
    /*
      if( rxTx != 0 ) // 1: TX, 0: RX
      {
      GpioWrite( &AntSwitchLf, 0 );
      GpioWrite( &AntSwitchHf, 1 );
      }
      else
      {
      GpioWrite( &AntSwitchLf, 1 );
      GpioWrite( &AntSwitchHf, 0 );
      }
    */
    if( rxTx != 0 ) // 1: TX, 0: RX
        {
            digitalWrite(RX_EN,0);
            digitalWrite(TX_EN,1);
        }
    else
        {
            digitalWrite(TX_EN,0);
            digitalWrite(RX_EN,1);
        }
}

bool SX1278CheckRfFrequency( uint32_t frequency )
{
    // Implement check. Currently all frequencies are supported
    return true;
}

void SX1278SetReset(void)
{
    pinMode(RESET_EN,OUTPUT);
    digitalWrite(RESET_EN,0);
    delay(1);
    pinMode(RESET_EN,INPUT);
    delay(6);
}

void SpiSetNss(uint8_t val)
{
    digitalWrite(NSS_SPI1,val);
}

uint8_t SpiInOut( uint8_t outData )
{
    SPI1.transfer(outData);
}

void SX1278BoardInit(void)
{
    SPI1.setBitOrder(MSBFIRST);
    SPI1.setClockDivider(SPI_CLOCK_DIV16);
    SPI1.setDataMode(0);
    SPI1.begin();
    SX1278IoInit();
}

void DelayMs(uint32_t ms)
{
    delay(ms);
}

uint8_t GetBoardPowerSource( void )
{
    // #if defined( USE_USB_CDC )
    //     if( GpioRead( &UsbDetect ) == 1 )
    //         {
    //             return BATTERY_POWER;
    //         }
    //     else
    //         {
    //             return USB_POWER;
    //         }
    // #else
    // return BATTERY_POWER;
    return 0;
    // #endif
}

uint8_t BoardGetBatteryLevel(void)
{
    return 200;
}

uint32_t BoardGetRandomSeed( void )
{
    return ( ( *( uint32_t* )ID1 ) ^ ( *( uint32_t* )ID2 ) ^ ( *( uint32_t* )ID3 ) );
}

void BoardGetUniqueId( uint8_t *id )
{
    id[7] = ( ( *( uint32_t* )ID1 )+ ( *( uint32_t* )ID3 ) ) >> 24;
    id[6] = ( ( *( uint32_t* )ID1 )+ ( *( uint32_t* )ID3 ) ) >> 16;
    id[5] = ( ( *( uint32_t* )ID1 )+ ( *( uint32_t* )ID3 ) ) >> 8;
    id[4] = ( ( *( uint32_t* )ID1 )+ ( *( uint32_t* )ID3 ) );
    id[3] = ( ( *( uint32_t* )ID2 ) ) >> 24;
    id[2] = ( ( *( uint32_t* )ID2 ) ) >> 16;
    id[1] = ( ( *( uint32_t* )ID2 ) ) >> 8;
    id[0] = ( ( *( uint32_t* )ID2 ) );
}
