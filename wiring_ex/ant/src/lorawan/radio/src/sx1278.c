/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech

Description: Generic SX1278 driver implementation

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis, Gregory Cristian and Wael Guibene
*/
#include <math.h>
#include <string.h>
#include "radio.h"
#include "sx1278.h"
#include "sx1278-board.h"
#include "timer.h"
#include "utilities.h"
#include "service_debug.h"
/*
 * Local types definition
 */

/*!
 * Radio registers definition
 */
typedef struct
{
    RadioModems_t Modem;
    uint8_t       Addr;
    uint8_t       Value;
}RadioRegisters_t;

/*!
 * FSK bandwidth definition
 */
typedef struct
{
    uint32_t bandwidth;
    uint8_t  RegValue;
}FskBandwidth_t;


/*
 * Private functions prototypes
 */

/*!
 * Performs the Rx chain calibration for LF and HF bands
 * \remark Must be called just after the reset so all registers are at their
 *         default values
 */
static void RxChainCalibration( void );

/*!
 * \brief Resets the SX1278
 */
void SX1278Reset( void );

/*!
 * \brief Sets the SX1278 in transmission mode for the given time
 * \param [IN] timeout Transmission timeout [ms] [0: continuous, others timeout]
 */
void SX1278SetTx( uint32_t timeout );

/*!
 * \brief Writes the buffer contents to the SX1278 FIFO
 *
 * \param [IN] buffer Buffer containing data to be put on the FIFO.
 * \param [IN] size Number of bytes to be written to the FIFO
 */
void SX1278WriteFifo( uint8_t *buffer, uint8_t size );

/*!
 * \brief Reads the contents of the SX1278 FIFO
 *
 * \param [OUT] buffer Buffer where to copy the FIFO read data.
 * \param [IN] size Number of bytes to be read from the FIFO
 */
void SX1278ReadFifo( uint8_t *buffer, uint8_t size );

/*!
 * \brief Sets the SX1278 operating mode
 *
 * \param [IN] opMode New operating mode
 */
void SX1278SetOpMode( uint8_t opMode );

/*
 * SX1278 DIO IRQ callback functions prototype
 */

/*!
 * \brief DIO 0 IRQ callback
 */
void SX1278OnDio0Irq( void );

/*!
 * \brief DIO 1 IRQ callback
 */
void SX1278OnDio1Irq( void );

/*!
 * \brief DIO 2 IRQ callback
 */
void SX1278OnDio2Irq( void );

/*!
 * \brief DIO 3 IRQ callback
 */
void SX1278OnDio3Irq( void );

/*!
 * \brief DIO 4 IRQ callback
 */
void SX1278OnDio4Irq( void );

/*!
 * \brief DIO 5 IRQ callback
 */
void SX1278OnDio5Irq( void );

/*!
 * \brief Tx & Rx timeout timer callback
 */
void SX1278OnTimeoutIrq( void );

/*
 * Private global constants
 */

/*!
 * Radio hardware registers initialization
 *
 * \remark RADIO_INIT_REGISTERS_VALUE is defined in sx1276-board.h file
 */
const RadioRegisters_t RadioRegsInit[] = RADIO_INIT_REGISTERS_VALUE;

/*!
 * Constant values need to compute the RSSI value
 */
#define RSSI_OFFSET_LF    -164
#define RSSI_OFFSET_HF    -157

/*!
 * Precomputed FSK bandwidth registers values
 */
const FskBandwidth_t FskBandwidths[] =
{
    { 2600  , 0x17 },
    { 3100  , 0x0F },
    { 3900  , 0x07 },
    { 5200  , 0x16 },
    { 6300  , 0x0E },
    { 7800  , 0x06 },
    { 10400 , 0x15 },
    { 12500 , 0x0D },
    { 15600 , 0x05 },
    { 20800 , 0x14 },
    { 25000 , 0x0C },
    { 31300 , 0x04 },
    { 41700 , 0x13 },
    { 50000 , 0x0B },
    { 62500 , 0x03 },
    { 83333 , 0x12 },
    { 100000, 0x0A },
    { 125000, 0x02 },
    { 166700, 0x11 },
    { 200000, 0x09 },
    { 250000, 0x01 },
    { 300000, 0x00 }, // Invalid Badwidth
};

/*
 * Private global variables
 */

/*!
 * Radio callbacks variable
 */
static RadioEvents_t *RadioEvents;

/*!
 * Reception buffer
 */
static uint8_t RxTxBuffer[RX_BUFFER_SIZE];

/*
 * Public global variables
 */

/*!
 * Radio hardware and global parameters
 */
SX1278_t SX1278;

/*!
 * Hardware DIO IRQ callback initialization
 */
DioIrqHandler *DioIrq[] = { SX1278OnDio0Irq, SX1278OnDio1Irq,
                            SX1278OnDio2Irq, SX1278OnDio3Irq,
                            SX1278OnDio4Irq, SX1278OnDio5Irq
                          };

/*!
 * Tx and Rx timers
 */
TimerEvent_t TxTimeoutTimer;
TimerEvent_t RxTimeoutTimer;
TimerEvent_t RxTimeoutSyncWord;


/*
 * Radio driver functions implementation
 */

void SX1278Init( RadioEvents_t *events )
{
    uint8_t i;

    RadioEvents = events;

    // Initialize driver timeout timers
    TimerInit( &TxTimeoutTimer, SX1278OnTimeoutIrq );
    TimerInit( &RxTimeoutTimer, SX1278OnTimeoutIrq );
    TimerInit( &RxTimeoutSyncWord, SX1278OnTimeoutIrq );

    SX1278Reset( );

    RxChainCalibration( );

    SX1278SetOpMode( RF_OPMODE_SLEEP );

    SX1278IoIrqInit( DioIrq );

    for( i = 0; i < sizeof( RadioRegsInit ) / sizeof( RadioRegisters_t ); i++ )
    {
        SX1278SetModem( RadioRegsInit[i].Modem );
        SX1278Write( RadioRegsInit[i].Addr, RadioRegsInit[i].Value );
    }

    SX1278SetModem( MODEM_FSK );

    SX1278.Settings.State = RF_IDLE;
}

RadioState_t SX1278GetStatus( void )
{
    return SX1278.Settings.State;
}

void SX1278SetChannel( uint32_t freq )
{
    SX1278.Settings.Channel = freq;
    freq = ( uint32_t )( ( double )freq / ( double )FREQ_STEP );
    SX1278Write( REG_FRFMSB, ( uint8_t )( ( freq >> 16 ) & 0xFF ) );
    SX1278Write( REG_FRFMID, ( uint8_t )( ( freq >> 8 ) & 0xFF ) );
    SX1278Write( REG_FRFLSB, ( uint8_t )( freq & 0xFF ) );
}

bool SX1278IsChannelFree( RadioModems_t modem, uint32_t freq, int16_t rssiThresh )
{
    int16_t rssi = 0;

    SX1278SetModem( modem );

    SX1278SetChannel( freq );

    SX1278SetOpMode( RF_OPMODE_RECEIVER );

    DelayMs( 1 );

    rssi = SX1278ReadRssi( modem );

    SX1278SetSleep( );

    if( rssi > rssiThresh )
    {
        return false;
    }
    return true;
}

uint32_t SX1278Random( void )
{
    uint8_t i;
    uint32_t rnd = 0;

    /*
     * Radio setup for random number generation
     */
    // Set LoRa modem ON
    SX1278SetModem( MODEM_LORA );

    // Disable LoRa modem interrupts
    SX1278Write( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
                  RFLR_IRQFLAGS_RXDONE |
                  RFLR_IRQFLAGS_PAYLOADCRCERROR |
                  RFLR_IRQFLAGS_VALIDHEADER |
                  RFLR_IRQFLAGS_TXDONE |
                  RFLR_IRQFLAGS_CADDONE |
                  RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                  RFLR_IRQFLAGS_CADDETECTED );

    // Set radio in continuous reception
    SX1278SetOpMode( RF_OPMODE_RECEIVER );

    for( i = 0; i < 32; i++ )
    {
        DelayMs( 1 );
        // Unfiltered RSSI value reading. Only takes the LSB value
        rnd |= ( ( uint32_t )SX1278Read( REG_LR_RSSIWIDEBAND ) & 0x01 ) << i;
    }

    SX1278SetSleep( );

    return rnd;
}

/*!
 * Performs the Rx chain calibration for LF and HF bands
 * \remark Must be called just after the reset so all registers are at their
 *         default values
 */
static void RxChainCalibration( void )
{
    uint8_t regPaConfigInitVal;
    uint32_t initialFreq;

    // Save context
    regPaConfigInitVal = SX1278Read( REG_PACONFIG );
    initialFreq = ( double )( ( ( uint32_t )SX1278Read( REG_FRFMSB ) << 16 ) |
                              ( ( uint32_t )SX1278Read( REG_FRFMID ) << 8 ) |
                              ( ( uint32_t )SX1278Read( REG_FRFLSB ) ) ) * ( double )FREQ_STEP;

    // Cut the PA just in case, RFO output, power = -1 dBm
    SX1278Write( REG_PACONFIG, 0x00 );

    // Launch Rx chain calibration for LF band
    SX1278Write( REG_IMAGECAL, ( SX1278Read( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_MASK ) | RF_IMAGECAL_IMAGECAL_START );
    while( ( SX1278Read( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_RUNNING ) == RF_IMAGECAL_IMAGECAL_RUNNING )
    {
    }

    // Sets a Frequency in HF band
    /* SX1278SetChannel( 868000000 ); */
    SX1278SetChannel( 434000000 );

    // Launch Rx chain calibration for HF band
    SX1278Write( REG_IMAGECAL, ( SX1278Read( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_MASK ) | RF_IMAGECAL_IMAGECAL_START );
    while( ( SX1278Read( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_RUNNING ) == RF_IMAGECAL_IMAGECAL_RUNNING )
    {
    }

    // Restore context
    SX1278Write( REG_PACONFIG, regPaConfigInitVal );
    SX1278SetChannel( initialFreq );
}

/*!
 * Returns the known FSK bandwidth registers value
 *
 * \param [IN] bandwidth Bandwidth value in Hz
 * \retval regValue Bandwidth register value.
 */
static uint8_t GetFskBandwidthRegValue( uint32_t bandwidth )
{
    uint8_t i;

    for( i = 0; i < ( sizeof( FskBandwidths ) / sizeof( FskBandwidth_t ) ) - 1; i++ )
    {
        if( ( bandwidth >= FskBandwidths[i].bandwidth ) && ( bandwidth < FskBandwidths[i + 1].bandwidth ) )
        {
            return FskBandwidths[i].RegValue;
        }
    }
    // ERROR: Value not found
    while( 1 );
}

void SX1278SetRxConfig( RadioModems_t modem, uint32_t bandwidth,
                         uint32_t datarate, uint8_t coderate,
                         uint32_t bandwidthAfc, uint16_t preambleLen,
                         uint16_t symbTimeout, bool fixLen,
                         uint8_t payloadLen,
                         bool crcOn, bool freqHopOn, uint8_t hopPeriod,
                         bool iqInverted, bool rxContinuous )
{
    SX1278SetModem( modem );

    switch( modem )
    {
    case MODEM_FSK:
        {
            SX1278.Settings.Fsk.Bandwidth = bandwidth;
            SX1278.Settings.Fsk.Datarate = datarate;
            SX1278.Settings.Fsk.BandwidthAfc = bandwidthAfc;
            SX1278.Settings.Fsk.FixLen = fixLen;
            SX1278.Settings.Fsk.PayloadLen = payloadLen;
            SX1278.Settings.Fsk.CrcOn = crcOn;
            SX1278.Settings.Fsk.IqInverted = iqInverted;
            SX1278.Settings.Fsk.RxContinuous = rxContinuous;
            SX1278.Settings.Fsk.PreambleLen = preambleLen;

            datarate = ( uint16_t )( ( double )XTAL_FREQ / ( double )datarate );
            SX1278Write( REG_BITRATEMSB, ( uint8_t )( datarate >> 8 ) );
            SX1278Write( REG_BITRATELSB, ( uint8_t )( datarate & 0xFF ) );

            SX1278Write( REG_RXBW, GetFskBandwidthRegValue( bandwidth ) );
            SX1278Write( REG_AFCBW, GetFskBandwidthRegValue( bandwidthAfc ) );

            SX1278Write( REG_PREAMBLEMSB, ( uint8_t )( ( preambleLen >> 8 ) & 0xFF ) );
            SX1278Write( REG_PREAMBLELSB, ( uint8_t )( preambleLen & 0xFF ) );

            if( fixLen == 1 )
            {
                SX1278Write( REG_PAYLOADLENGTH, payloadLen );
            }
            else
            {
                SX1278Write( REG_PAYLOADLENGTH, 0xFF ); // Set payload length to the maximum
            }

            SX1278Write( REG_PACKETCONFIG1,
                         ( SX1278Read( REG_PACKETCONFIG1 ) &
                           RF_PACKETCONFIG1_CRC_MASK &
                           RF_PACKETCONFIG1_PACKETFORMAT_MASK ) |
                           ( ( fixLen == 1 ) ? RF_PACKETCONFIG1_PACKETFORMAT_FIXED : RF_PACKETCONFIG1_PACKETFORMAT_VARIABLE ) |
                           ( crcOn << 4 ) );
        }
        break;
    case MODEM_LORA:
        {
            if( bandwidth > 2 )
            {
                // Fatal error: When using LoRa modem only bandwidths 125, 250 and 500 kHz are supported
                while( 1 );
            }
            bandwidth += 7;
            SX1278.Settings.LoRa.Bandwidth = bandwidth;
            SX1278.Settings.LoRa.Datarate = datarate;
            SX1278.Settings.LoRa.Coderate = coderate;
            SX1278.Settings.LoRa.PreambleLen = preambleLen;
            SX1278.Settings.LoRa.FixLen = fixLen;
            SX1278.Settings.LoRa.PayloadLen = payloadLen;
            SX1278.Settings.LoRa.CrcOn = crcOn;
            SX1278.Settings.LoRa.FreqHopOn = freqHopOn;
            SX1278.Settings.LoRa.HopPeriod = hopPeriod;
            SX1278.Settings.LoRa.IqInverted = iqInverted;
            SX1278.Settings.LoRa.RxContinuous = rxContinuous;

            if( datarate > 12 )
            {
                datarate = 12;
            }
            else if( datarate < 6 )
            {
                datarate = 6;
            }

            if( ( ( bandwidth == 7 ) && ( ( datarate == 11 ) || ( datarate == 12 ) ) ) ||
                ( ( bandwidth == 8 ) && ( datarate == 12 ) ) )
            {
                SX1278.Settings.LoRa.LowDatarateOptimize = 0x01;
            }
            else
            {
                SX1278.Settings.LoRa.LowDatarateOptimize = 0x00;
            }

            SX1278Write( REG_LR_MODEMCONFIG1,
                         ( SX1278Read( REG_LR_MODEMCONFIG1 ) &
                           RFLR_MODEMCONFIG1_BW_MASK &
                           RFLR_MODEMCONFIG1_CODINGRATE_MASK &
                           RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK ) |
                           ( bandwidth << 4 ) | ( coderate << 1 ) |
                           fixLen );

            SX1278Write( REG_LR_MODEMCONFIG2,
                         ( SX1278Read( REG_LR_MODEMCONFIG2 ) &
                           RFLR_MODEMCONFIG2_SF_MASK &
                           RFLR_MODEMCONFIG2_RXPAYLOADCRC_MASK &
                           RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) |
                           ( datarate << 4 ) | ( crcOn << 2 ) |
                           ( ( symbTimeout >> 8 ) & ~RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) );

            SX1278Write( REG_LR_MODEMCONFIG3,
                         ( SX1278Read( REG_LR_MODEMCONFIG3 ) &
                           RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_MASK ) |
                           ( SX1278.Settings.LoRa.LowDatarateOptimize << 3 ) );

            SX1278Write( REG_LR_SYMBTIMEOUTLSB, ( uint8_t )( symbTimeout & 0xFF ) );

            SX1278Write( REG_LR_PREAMBLEMSB, ( uint8_t )( ( preambleLen >> 8 ) & 0xFF ) );
            SX1278Write( REG_LR_PREAMBLELSB, ( uint8_t )( preambleLen & 0xFF ) );

            if( fixLen == 1 )
            {
                SX1278Write( REG_LR_PAYLOADLENGTH, payloadLen );
            }

            if( SX1278.Settings.LoRa.FreqHopOn == true )
            {
                SX1278Write( REG_LR_PLLHOP, ( SX1278Read( REG_LR_PLLHOP ) & RFLR_PLLHOP_FASTHOP_MASK ) | RFLR_PLLHOP_FASTHOP_ON );
                SX1278Write( REG_LR_HOPPERIOD, SX1278.Settings.LoRa.HopPeriod );
            }

            if( ( bandwidth == 9 ) && ( SX1278.Settings.Channel > RF_MID_BAND_THRESH ) )
            {
                // ERRATA 2.1 - Sensitivity Optimization with a 500 kHz Bandwidth
                SX1278Write( REG_LR_TEST36, 0x02 );
                SX1278Write( REG_LR_TEST3A, 0x64 );
            }
            else if( bandwidth == 9 )
            {
                // ERRATA 2.1 - Sensitivity Optimization with a 500 kHz Bandwidth
                SX1278Write( REG_LR_TEST36, 0x02 );
                SX1278Write( REG_LR_TEST3A, 0x7F );
            }
            else
            {
                // ERRATA 2.1 - Sensitivity Optimization with a 500 kHz Bandwidth
                SX1278Write( REG_LR_TEST36, 0x03 );
            }

            if( datarate == 6 )
            {
                SX1278Write( REG_LR_DETECTOPTIMIZE,
                             ( SX1278Read( REG_LR_DETECTOPTIMIZE ) &
                               RFLR_DETECTIONOPTIMIZE_MASK ) |
                               RFLR_DETECTIONOPTIMIZE_SF6 );
                SX1278Write( REG_LR_DETECTIONTHRESHOLD,
                             RFLR_DETECTIONTHRESH_SF6 );
            }
            else
            {
                SX1278Write( REG_LR_DETECTOPTIMIZE,
                             ( SX1278Read( REG_LR_DETECTOPTIMIZE ) &
                             RFLR_DETECTIONOPTIMIZE_MASK ) |
                             RFLR_DETECTIONOPTIMIZE_SF7_TO_SF12 );
                SX1278Write( REG_LR_DETECTIONTHRESHOLD,
                             RFLR_DETECTIONTHRESH_SF7_TO_SF12 );
            }
        }
        break;
    }
}

void SX1278SetTxConfig( RadioModems_t modem, int8_t power, uint32_t fdev,
                        uint32_t bandwidth, uint32_t datarate,
                        uint8_t coderate, uint16_t preambleLen,
                        bool fixLen, bool crcOn, bool freqHopOn,
                        uint8_t hopPeriod, bool iqInverted, uint32_t timeout )
{
    uint8_t paConfig = 0;
    uint8_t paDac = 0;

    SX1278SetModem( modem );

    paConfig = SX1278Read( REG_PACONFIG );
    paDac = SX1278Read( REG_PADAC );

    paConfig = ( paConfig & RF_PACONFIG_PASELECT_MASK ) | SX1278GetPaSelect( SX1278.Settings.Channel );
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
    SX1278Write( REG_PACONFIG, paConfig );
    SX1278Write( REG_PADAC, paDac );

    switch( modem )
    {
    case MODEM_FSK:
        {
            SX1278.Settings.Fsk.Power = power;
            SX1278.Settings.Fsk.Fdev = fdev;
            SX1278.Settings.Fsk.Bandwidth = bandwidth;
            SX1278.Settings.Fsk.Datarate = datarate;
            SX1278.Settings.Fsk.PreambleLen = preambleLen;
            SX1278.Settings.Fsk.FixLen = fixLen;
            SX1278.Settings.Fsk.CrcOn = crcOn;
            SX1278.Settings.Fsk.IqInverted = iqInverted;
            SX1278.Settings.Fsk.TxTimeout = timeout;

            fdev = ( uint16_t )( ( double )fdev / ( double )FREQ_STEP );
            SX1278Write( REG_FDEVMSB, ( uint8_t )( fdev >> 8 ) );
            SX1278Write( REG_FDEVLSB, ( uint8_t )( fdev & 0xFF ) );

            datarate = ( uint16_t )( ( double )XTAL_FREQ / ( double )datarate );
            SX1278Write( REG_BITRATEMSB, ( uint8_t )( datarate >> 8 ) );
            SX1278Write( REG_BITRATELSB, ( uint8_t )( datarate & 0xFF ) );


            SX1278Write( REG_PREAMBLELSB, preambleLen & 0xFF );

            SX1278Write( REG_PACKETCONFIG1,
                         ( SX1278Read( REG_PACKETCONFIG1 ) &
                           RF_PACKETCONFIG1_CRC_MASK &
                           RF_PACKETCONFIG1_PACKETFORMAT_MASK ) |
                           ( ( fixLen == 1 ) ? RF_PACKETCONFIG1_PACKETFORMAT_FIXED : RF_PACKETCONFIG1_PACKETFORMAT_VARIABLE ) |
                           ( crcOn << 4 ) );

        }
        break;
    case MODEM_LORA:
        {
            SX1278.Settings.LoRa.Power = power;
            if( bandwidth > 2 )
            {
                // Fatal error: When using LoRa modem only bandwidths 125, 250 and 500 kHz are supported
                while( 1 );
            }
            bandwidth += 7;
            SX1278.Settings.LoRa.Bandwidth = bandwidth;
            SX1278.Settings.LoRa.Datarate = datarate;
            SX1278.Settings.LoRa.Coderate = coderate;
            SX1278.Settings.LoRa.PreambleLen = preambleLen;
            SX1278.Settings.LoRa.FixLen = fixLen;
            SX1278.Settings.LoRa.FreqHopOn = freqHopOn;
            SX1278.Settings.LoRa.HopPeriod = hopPeriod;
            SX1278.Settings.LoRa.CrcOn = crcOn;
            SX1278.Settings.LoRa.IqInverted = iqInverted;
            SX1278.Settings.LoRa.TxTimeout = timeout;

            if( datarate > 12 )
            {
                datarate = 12;
            }
            else if( datarate < 6 )
            {
                datarate = 6;
            }
            if( ( ( bandwidth == 7 ) && ( ( datarate == 11 ) || ( datarate == 12 ) ) ) ||
                ( ( bandwidth == 8 ) && ( datarate == 12 ) ) )
            {
                SX1278.Settings.LoRa.LowDatarateOptimize = 0x01;
            }
            else
            {
                SX1278.Settings.LoRa.LowDatarateOptimize = 0x00;
            }

            if( SX1278.Settings.LoRa.FreqHopOn == true )
            {
                SX1278Write( REG_LR_PLLHOP, ( SX1278Read( REG_LR_PLLHOP ) & RFLR_PLLHOP_FASTHOP_MASK ) | RFLR_PLLHOP_FASTHOP_ON );
                SX1278Write( REG_LR_HOPPERIOD, SX1278.Settings.LoRa.HopPeriod );
            }

            SX1278Write( REG_LR_MODEMCONFIG1,
                         ( SX1278Read( REG_LR_MODEMCONFIG1 ) &
                           RFLR_MODEMCONFIG1_BW_MASK &
                           RFLR_MODEMCONFIG1_CODINGRATE_MASK &
                           RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK ) |
                           ( bandwidth << 4 ) | ( coderate << 1 ) |
                           fixLen );

            SX1278Write( REG_LR_MODEMCONFIG2,
                         ( SX1278Read( REG_LR_MODEMCONFIG2 ) &
                           RFLR_MODEMCONFIG2_SF_MASK &
                           RFLR_MODEMCONFIG2_RXPAYLOADCRC_MASK ) |
                           ( datarate << 4 ) | ( crcOn << 2 ) );

            SX1278Write( REG_LR_MODEMCONFIG3,
                         ( SX1278Read( REG_LR_MODEMCONFIG3 ) &
                           RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_MASK ) |
                           ( SX1278.Settings.LoRa.LowDatarateOptimize << 3 ) );

            SX1278Write( REG_LR_PREAMBLEMSB, ( preambleLen >> 8 ) & 0x00FF );
            SX1278Write( REG_LR_PREAMBLELSB, preambleLen & 0xFF );

            if( datarate == 6 )
            {
                SX1278Write( REG_LR_DETECTOPTIMIZE,
                             ( SX1278Read( REG_LR_DETECTOPTIMIZE ) &
                               RFLR_DETECTIONOPTIMIZE_MASK ) |
                               RFLR_DETECTIONOPTIMIZE_SF6 );
                SX1278Write( REG_LR_DETECTIONTHRESHOLD,
                             RFLR_DETECTIONTHRESH_SF6 );
            }
            else
            {
                SX1278Write( REG_LR_DETECTOPTIMIZE,
                             ( SX1278Read( REG_LR_DETECTOPTIMIZE ) &
                             RFLR_DETECTIONOPTIMIZE_MASK ) |
                             RFLR_DETECTIONOPTIMIZE_SF7_TO_SF12 );
                SX1278Write( REG_LR_DETECTIONTHRESHOLD,
                             RFLR_DETECTIONTHRESH_SF7_TO_SF12 );
            }
        }
        break;
    }
}

uint32_t SX1278GetTimeOnAir( RadioModems_t modem, uint8_t pktLen )
{
    uint32_t airTime = 0;

    switch( modem )
    {
    case MODEM_FSK:
        {
            airTime = round( ( 8 * ( SX1278.Settings.Fsk.PreambleLen +
                                     ( ( SX1278Read( REG_SYNCCONFIG ) & ~RF_SYNCCONFIG_SYNCSIZE_MASK ) + 1 ) +
                                     ( ( SX1278.Settings.Fsk.FixLen == 0x01 ) ? 0.0 : 1.0 ) +
                                     ( ( ( SX1278Read( REG_PACKETCONFIG1 ) & ~RF_PACKETCONFIG1_ADDRSFILTERING_MASK ) != 0x00 ) ? 1.0 : 0 ) +
                                     pktLen +
                                     ( ( SX1278.Settings.Fsk.CrcOn == 0x01 ) ? 2.0 : 0 ) ) /
                                     SX1278.Settings.Fsk.Datarate ) * 1e3 );
        }
        break;
    case MODEM_LORA:
        {
            double bw = 0.0;
            // REMARK: When using LoRa modem only bandwidths 125, 250 and 500 kHz are supported
            switch( SX1278.Settings.LoRa.Bandwidth )
            {
            //case 0: // 7.8 kHz
            //    bw = 78e2;
            //    break;
            //case 1: // 10.4 kHz
            //    bw = 104e2;
            //    break;
            //case 2: // 15.6 kHz
            //    bw = 156e2;
            //    break;
            //case 3: // 20.8 kHz
            //    bw = 208e2;
            //    break;
            //case 4: // 31.2 kHz
            //    bw = 312e2;
            //    break;
            //case 5: // 41.4 kHz
            //    bw = 414e2;
            //    break;
            //case 6: // 62.5 kHz
            //    bw = 625e2;
            //    break;
            case 7: // 125 kHz
                bw = 125e3;
                break;
            case 8: // 250 kHz
                bw = 250e3;
                break;
            case 9: // 500 kHz
                bw = 500e3;
                break;
            }

            // Symbol rate : time for one symbol (secs)
            double rs = bw / ( 1 << SX1278.Settings.LoRa.Datarate );
            double ts = 1 / rs;
            // time of preamble
            double tPreamble = ( SX1278.Settings.LoRa.PreambleLen + 4.25 ) * ts;
            // Symbol length of payload and time
            double tmp = ceil( ( 8 * pktLen - 4 * SX1278.Settings.LoRa.Datarate +
                                 28 + 16 * SX1278.Settings.LoRa.CrcOn -
                                 ( SX1278.Settings.LoRa.FixLen ? 20 : 0 ) ) /
                                 ( double )( 4 * SX1278.Settings.LoRa.Datarate -
                                 ( ( SX1278.Settings.LoRa.LowDatarateOptimize > 0 ) ? 2 : 0 ) ) ) *
                                 ( SX1278.Settings.LoRa.Coderate + 4 );
            double nPayload = 8 + ( ( tmp > 0 ) ? tmp : 0 );
            double tPayload = nPayload * ts;
            // Time on air
            double tOnAir = tPreamble + tPayload;
            // return us secs
            airTime = floor( tOnAir * 1e3 + 0.999 );
        }
        break;
    }
    return airTime;
}

void SX1278Send( uint8_t *buffer, uint8_t size )
{
    uint32_t txTimeout = 0;

    switch( SX1278.Settings.Modem )
    {
    case MODEM_FSK:
        {
            SX1278.Settings.FskPacketHandler.NbBytes = 0;
            SX1278.Settings.FskPacketHandler.Size = size;

            if( SX1278.Settings.Fsk.FixLen == false )
            {
                SX1278WriteFifo( ( uint8_t* )&size, 1 );
            }
            else
            {
                SX1278Write( REG_PAYLOADLENGTH, size );
            }

            if( ( size > 0 ) && ( size <= 64 ) )
            {
                SX1278.Settings.FskPacketHandler.ChunkSize = size;
            }
            else
            {
                memcpy1( RxTxBuffer, buffer, size );
                SX1278.Settings.FskPacketHandler.ChunkSize = 32;
            }

            // Write payload buffer
            SX1278WriteFifo( buffer, SX1278.Settings.FskPacketHandler.ChunkSize );
            SX1278.Settings.FskPacketHandler.NbBytes += SX1278.Settings.FskPacketHandler.ChunkSize;
            txTimeout = SX1278.Settings.Fsk.TxTimeout;
        }
        break;
    case MODEM_LORA:
        {
            if( SX1278.Settings.LoRa.IqInverted == true )
            {
                SX1278Write( REG_LR_INVERTIQ, ( ( SX1278Read( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_ON ) );
                SX1278Write( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_ON );

            }
            else
            {
                SX1278Write( REG_LR_INVERTIQ, ( ( SX1278Read( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_OFF ) );
                SX1278Write( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_OFF );

            }

            SX1278.Settings.LoRaPacketHandler.Size = size;

            // Initializes the payload size
            SX1278Write( REG_LR_PAYLOADLENGTH, size );

            // Full buffer used for Tx
            SX1278Write( REG_LR_FIFOTXBASEADDR, 0 );
            SX1278Write( REG_LR_FIFOADDRPTR, 0 );

            // FIFO operations can not take place in Sleep mode
            if( ( SX1278Read( REG_OPMODE ) & ~RF_OPMODE_MASK ) == RF_OPMODE_SLEEP )
            {
                SX1278SetStby( );
                DelayMs( 1 );
            }
            // Write payload buffer
            SX1278WriteFifo( buffer, size );
            txTimeout = SX1278.Settings.LoRa.TxTimeout;

        }
        break;
    }

    SX1278SetTx( txTimeout );

}

void SX1278SetSleep( void )
{
    TimerStop( &RxTimeoutTimer );
    TimerStop( &TxTimeoutTimer );

    SX1278SetOpMode( RF_OPMODE_SLEEP );
    SX1278.Settings.State = RF_IDLE;

    /* DEBUG("set 1278 sleep"); */
}

void SX1278SetStby( void )
{
    TimerStop( &RxTimeoutTimer );
    TimerStop( &TxTimeoutTimer );

    SX1278SetOpMode( RF_OPMODE_STANDBY );
    SX1278.Settings.State = RF_IDLE;

    DEBUG("set 1278 stdby");
}

void SX1278SetRx( uint32_t timeout )
{
    bool rxContinuous = false;

    switch( SX1278.Settings.Modem )
    {
    case MODEM_FSK:
        {
            rxContinuous = SX1278.Settings.Fsk.RxContinuous;

            // DIO0=PayloadReady
            // DIO1=FifoLevel
            // DIO2=SyncAddr
            // DIO3=FifoEmpty
            // DIO4=Preamble
            // DIO5=ModeReady
            SX1278Write( REG_DIOMAPPING1, ( SX1278Read( REG_DIOMAPPING1 ) & RF_DIOMAPPING1_DIO0_MASK &
                                                                            RF_DIOMAPPING1_DIO1_MASK &
                                                                            RF_DIOMAPPING1_DIO2_MASK ) |
                                                                            RF_DIOMAPPING1_DIO0_00 |
                                                                            RF_DIOMAPPING1_DIO1_00 |
                                                                            RF_DIOMAPPING1_DIO2_11 );

            SX1278Write( REG_DIOMAPPING2, ( SX1278Read( REG_DIOMAPPING2 ) & RF_DIOMAPPING2_DIO4_MASK &
                                                                            RF_DIOMAPPING2_MAP_MASK ) |
                                                                            RF_DIOMAPPING2_DIO4_11 |
                                                                            RF_DIOMAPPING2_MAP_PREAMBLEDETECT );

            SX1278.Settings.FskPacketHandler.FifoThresh = SX1278Read( REG_FIFOTHRESH ) & 0x3F;

            SX1278Write( REG_RXCONFIG, RF_RXCONFIG_AFCAUTO_ON | RF_RXCONFIG_AGCAUTO_ON | RF_RXCONFIG_RXTRIGER_PREAMBLEDETECT );

            SX1278.Settings.FskPacketHandler.PreambleDetected = false;
            SX1278.Settings.FskPacketHandler.SyncWordDetected = false;
            SX1278.Settings.FskPacketHandler.NbBytes = 0;
            SX1278.Settings.FskPacketHandler.Size = 0;
        }
        break;
    case MODEM_LORA:
        {
            if( SX1278.Settings.LoRa.IqInverted == true )
            {
                SX1278Write( REG_LR_INVERTIQ, ( ( SX1278Read( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_ON | RFLR_INVERTIQ_TX_OFF ) );
                SX1278Write( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_ON );
            }
            else
            {
                SX1278Write( REG_LR_INVERTIQ, ( ( SX1278Read( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_OFF ) );
                SX1278Write( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_OFF );
            }

            // ERRATA 2.3 - Receiver Spurious Reception of a LoRa Signal
            if( SX1278.Settings.LoRa.Bandwidth < 9 )
            {
                SX1278Write( REG_LR_DETECTOPTIMIZE, SX1278Read( REG_LR_DETECTOPTIMIZE ) & 0x7F );
                SX1278Write( REG_LR_TEST30, 0x00 );
                switch( SX1278.Settings.LoRa.Bandwidth )
                {
                case 0: // 7.8 kHz
                    SX1278Write( REG_LR_TEST2F, 0x48 );
                    SX1278SetChannel(SX1278.Settings.Channel + 7.81e3 );
                    break;
                case 1: // 10.4 kHz
                    SX1278Write( REG_LR_TEST2F, 0x44 );
                    SX1278SetChannel(SX1278.Settings.Channel + 10.42e3 );
                    break;
                case 2: // 15.6 kHz
                    SX1278Write( REG_LR_TEST2F, 0x44 );
                    SX1278SetChannel(SX1278.Settings.Channel + 15.62e3 );
                    break;
                case 3: // 20.8 kHz
                    SX1278Write( REG_LR_TEST2F, 0x44 );
                    SX1278SetChannel(SX1278.Settings.Channel + 20.83e3 );
                    break;
                case 4: // 31.2 kHz
                    SX1278Write( REG_LR_TEST2F, 0x44 );
                    SX1278SetChannel(SX1278.Settings.Channel + 31.25e3 );
                    break;
                case 5: // 41.4 kHz
                    SX1278Write( REG_LR_TEST2F, 0x44 );
                    SX1278SetChannel(SX1278.Settings.Channel + 41.67e3 );
                    break;
                case 6: // 62.5 kHz
                    SX1278Write( REG_LR_TEST2F, 0x40 );
                    break;
                case 7: // 125 kHz
                    SX1278Write( REG_LR_TEST2F, 0x40 );
                    break;
                case 8: // 250 kHz
                    SX1278Write( REG_LR_TEST2F, 0x40 );
                    break;
                }
            }
            else
            {
                SX1278Write( REG_LR_DETECTOPTIMIZE, SX1278Read( REG_LR_DETECTOPTIMIZE ) | 0x80 );
            }

            rxContinuous = SX1278.Settings.LoRa.RxContinuous;

            if( SX1278.Settings.LoRa.FreqHopOn == true )
            {
                SX1278Write( REG_LR_IRQFLAGSMASK, //RFLR_IRQFLAGS_RXTIMEOUT |
                                                  //RFLR_IRQFLAGS_RXDONE |
                                                  //RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                                  RFLR_IRQFLAGS_VALIDHEADER |
                                                  RFLR_IRQFLAGS_TXDONE |
                                                  RFLR_IRQFLAGS_CADDONE |
                                                  //RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                                  RFLR_IRQFLAGS_CADDETECTED );

                // DIO0=RxDone, DIO2=FhssChangeChannel
                SX1278Write( REG_DIOMAPPING1, ( SX1278Read( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK & RFLR_DIOMAPPING1_DIO2_MASK  ) | RFLR_DIOMAPPING1_DIO0_00 | RFLR_DIOMAPPING1_DIO2_00 );
            }
            else
            {
                SX1278Write( REG_LR_IRQFLAGSMASK, //RFLR_IRQFLAGS_RXTIMEOUT |
                                                  //RFLR_IRQFLAGS_RXDONE |
                                                  //RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                                  RFLR_IRQFLAGS_VALIDHEADER |
                                                  RFLR_IRQFLAGS_TXDONE |
                                                  RFLR_IRQFLAGS_CADDONE |
                                                  // RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                                  RFLR_IRQFLAGS_CADDETECTED );

                // DIO0=RxDone
                SX1278Write( REG_DIOMAPPING1, ( SX1278Read( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK ) | RFLR_DIOMAPPING1_DIO0_00 );
            }
            SX1278Write( REG_LR_FIFORXBASEADDR, 0 );
            SX1278Write( REG_LR_FIFOADDRPTR, 0 );
        }
        break;
    }

    memset( RxTxBuffer, 0, ( size_t )RX_BUFFER_SIZE );

    SX1278.Settings.State = RF_RX_RUNNING;
    if( timeout != 0 )
    {
        TimerSetValue( &RxTimeoutTimer, timeout );
        TimerStart( &RxTimeoutTimer );
    }

    if( SX1278.Settings.Modem == MODEM_FSK )
    {
        SX1278SetOpMode( RF_OPMODE_RECEIVER );

        if( rxContinuous == false )
        {
            TimerSetValue( &RxTimeoutSyncWord, ceil( ( 8.0 * ( SX1278.Settings.Fsk.PreambleLen +
                                                             ( ( SX1278Read( REG_SYNCCONFIG ) &
                                                                ~RF_SYNCCONFIG_SYNCSIZE_MASK ) +
                                                                1.0 ) + 10.0 ) /
                                                             ( double )SX1278.Settings.Fsk.Datarate ) * 1e3 ) + 4 );
            TimerStart( &RxTimeoutSyncWord );
        }
    }
    else
    {
        if( rxContinuous == true )
        {
            SX1278SetOpMode( RFLR_OPMODE_RECEIVER );
        }
        else
        {
            SX1278SetOpMode( RFLR_OPMODE_RECEIVER_SINGLE );
        }
    }

}

void SX1278SetTx( uint32_t timeout )
{
    TimerSetValue( &TxTimeoutTimer, timeout );

    switch( SX1278.Settings.Modem )
    {
    case MODEM_FSK:
        {
            // DIO0=PacketSent
            // DIO1=FifoEmpty
            // DIO2=FifoFull
            // DIO3=FifoEmpty
            // DIO4=LowBat
            // DIO5=ModeReady
            SX1278Write( REG_DIOMAPPING1, ( SX1278Read( REG_DIOMAPPING1 ) & RF_DIOMAPPING1_DIO0_MASK &
                                                                            RF_DIOMAPPING1_DIO1_MASK &
                                                                            RF_DIOMAPPING1_DIO2_MASK ) |
                                                                            RF_DIOMAPPING1_DIO1_01 );

            SX1278Write( REG_DIOMAPPING2, ( SX1278Read( REG_DIOMAPPING2 ) & RF_DIOMAPPING2_DIO4_MASK &
                                                                            RF_DIOMAPPING2_MAP_MASK ) );
            SX1278.Settings.FskPacketHandler.FifoThresh = SX1278Read( REG_FIFOTHRESH ) & 0x3F;
        }
        break;
    case MODEM_LORA:
        {
            if( SX1278.Settings.LoRa.FreqHopOn == true )
            {
                SX1278Write( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
                                                  RFLR_IRQFLAGS_RXDONE |
                                                  RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                                  RFLR_IRQFLAGS_VALIDHEADER |
                                                  //RFLR_IRQFLAGS_TXDONE |
                                                  RFLR_IRQFLAGS_CADDONE |
                                                  //RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                                  RFLR_IRQFLAGS_CADDETECTED );

                // DIO0=TxDone, DIO2=FhssChangeChannel
                SX1278Write( REG_DIOMAPPING1, ( SX1278Read( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK & RFLR_DIOMAPPING1_DIO2_MASK ) | RFLR_DIOMAPPING1_DIO0_01 | RFLR_DIOMAPPING1_DIO2_00 );
            }
            else
            {
                SX1278Write( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
                                                  RFLR_IRQFLAGS_RXDONE |
                                                  RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                                  RFLR_IRQFLAGS_VALIDHEADER |
                                                  //RFLR_IRQFLAGS_TXDONE |
                                                  RFLR_IRQFLAGS_CADDONE |
                                                  RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                                  RFLR_IRQFLAGS_CADDETECTED );

                // DIO0=TxDone
                SX1278Write( REG_DIOMAPPING1, ( SX1278Read( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK ) | RFLR_DIOMAPPING1_DIO0_01 );
            }
        }
        break;
    }

    SX1278.Settings.State = RF_TX_RUNNING;
    TimerStart( &TxTimeoutTimer );
    SX1278SetOpMode( RF_OPMODE_TRANSMITTER );
}

void SX1278StartCad( void )
{
    switch( SX1278.Settings.Modem )
    {
    case MODEM_FSK:
        {

        }
        break;
    case MODEM_LORA:
        {
            SX1278Write( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
                                        RFLR_IRQFLAGS_RXDONE |
                                        RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                        RFLR_IRQFLAGS_VALIDHEADER |
                                        RFLR_IRQFLAGS_TXDONE |
                                        //RFLR_IRQFLAGS_CADDONE |
                                        RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL // |
                                        //RFLR_IRQFLAGS_CADDETECTED
                                        );

            // DIO3=CADDone
            SX1278Write( REG_DIOMAPPING1, ( SX1278Read( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK ) | RFLR_DIOMAPPING1_DIO0_00 );

            SX1278.Settings.State = RF_CAD;
            SX1278SetOpMode( RFLR_OPMODE_CAD );
        }
        break;
    default:
        break;
    }
}

int16_t SX1278ReadRssi( RadioModems_t modem )
{
    int16_t rssi = 0;

    switch( modem )
    {
    case MODEM_FSK:
        rssi = -( SX1278Read( REG_RSSIVALUE ) >> 1 );
        break;
    case MODEM_LORA:
        if( SX1278.Settings.Channel > RF_MID_BAND_THRESH )
        {
            rssi = RSSI_OFFSET_HF + SX1278Read( REG_LR_RSSIVALUE );
        }
        else
        {
            rssi = RSSI_OFFSET_LF + SX1278Read( REG_LR_RSSIVALUE );
        }
        break;
    default:
        rssi = -1;
        break;
    }
    return rssi;
}

void SX1278Reset( void )
{
    SX1278SetReset();
    /* // Set RESET pin to 0 */
    /* GpioInit( &SX1278.Reset, RADIO_RESET, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 ); */

    /* // Wait 1 ms */
    /* DelayMs( 1 ); */

    /* // Configure RESET as input */
    /* GpioInit( &SX1278.Reset, RADIO_RESET, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 1 ); */

    /* // Wait 6 ms */
    /* DelayMs( 6 ); */
}

void SX1278SetOpMode( uint8_t opMode )
{
    if( opMode == RF_OPMODE_SLEEP )
    {
        SX1278SetAntSwLowPower( true );
    }
    else
    {
        SX1278SetAntSwLowPower( false );
        if( opMode == RF_OPMODE_TRANSMITTER )
        {
            SX1278SetAntSw( 1 );
        }
        else
        {
            SX1278SetAntSw( 0 );
        }
    }

    SX1278Write( REG_OPMODE, ( SX1278Read( REG_OPMODE ) & RF_OPMODE_MASK ) | opMode );
}

void SX1278SetModem( RadioModems_t modem )
{
    //assert_param( ( SX1278.Spi.Spi.Instance != NULL ) );

    if( SX1278.Settings.Modem == modem )
    {
        return;
    }

    SX1278.Settings.Modem = modem;
    switch( SX1278.Settings.Modem )
    {
    default:
    case MODEM_FSK:
        SX1278SetOpMode( RF_OPMODE_SLEEP );
        SX1278Write( REG_OPMODE, ( SX1278Read( REG_OPMODE ) & RFLR_OPMODE_LONGRANGEMODE_MASK ) | RFLR_OPMODE_LONGRANGEMODE_OFF );

        SX1278Write( REG_DIOMAPPING1, 0x00 );
        SX1278Write( REG_DIOMAPPING2, 0x30 ); // DIO5=ModeReady
        break;
    case MODEM_LORA:
        SX1278SetOpMode( RF_OPMODE_SLEEP );
        SX1278Write( REG_OPMODE, ( SX1278Read( REG_OPMODE ) & RFLR_OPMODE_LONGRANGEMODE_MASK ) | RFLR_OPMODE_LONGRANGEMODE_ON );

        SX1278Write( REG_DIOMAPPING1, 0x00 );
        SX1278Write( REG_DIOMAPPING2, 0x00 );
        break;
    }
}

void SX1278Write( uint8_t addr, uint8_t data )
{
    SX1278WriteBuffer( addr, &data, 1 );
}

uint8_t SX1278Read( uint8_t addr )
{
    uint8_t data;
    SX1278ReadBuffer( addr, &data, 1 );
    return data;
}

void SX1278WriteBuffer( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;

    SpiSetNss(0);

    SpiInOut( addr | 0x80 );
    for( i = 0; i < size; i++ )
    {
        SpiInOut( buffer[i] );
    }

    SpiSetNss(1);
   /* //NSS = 0; */
    /* GpioWrite( &SX1278.Spi.Nss, 0 ); */

    /* SpiInOut( &SX1278.Spi, addr | 0x80 ); */
    /* for( i = 0; i < size; i++ ) */
    /* { */
    /*     SpiInOut( &SX1278.Spi, buffer[i] ); */
    /* } */

    /* //NSS = 1; */
    /* GpioWrite( &SX1278.Spi.Nss, 1 ); */
}

void SX1278ReadBuffer( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;

    SpiSetNss(0);

    SpiInOut( addr & 0x7F );
    for( i = 0; i < size; i++ )
    {
        buffer[i] = SpiInOut( 0 );
    }

    SpiSetNss(1);
    /* //NSS = 0; */
    /* GpioWrite( &SX1278.Spi.Nss, 0 ); */

    /* SpiInOut( &SX1278.Spi, addr & 0x7F ); */

    /* for( i = 0; i < size; i++ ) */
    /* { */
    /*     buffer[i] = SpiInOut( &SX1278.Spi, 0 ); */
    /* } */

    /* //NSS = 1; */
    /* GpioWrite( &SX1278.Spi.Nss, 1 ); */
}

void SX1278WriteFifo( uint8_t *buffer, uint8_t size )
{
    SX1278WriteBuffer( 0, buffer, size );
}

void SX1278ReadFifo( uint8_t *buffer, uint8_t size )
{
    SX1278ReadBuffer( 0, buffer, size );
}

void SX1278SetMaxPayloadLength( RadioModems_t modem, uint8_t max )
{
    SX1278SetModem( modem );

    switch( modem )
    {
    case MODEM_FSK:
        if( SX1278.Settings.Fsk.FixLen == false )
        {
            SX1278Write( REG_PAYLOADLENGTH, max );
        }
        break;
    case MODEM_LORA:
        DEBUG("max payload len = %d",max);
        SX1278Write( REG_LR_PAYLOADMAXLENGTH, max );
        break;
    }
}

void SX1278OnTimeoutIrq( void )
{

    DEBUG("into sx1278 on time out irq");

    switch( SX1278.Settings.State )
    {
    case RF_RX_RUNNING:
        if( SX1278.Settings.Modem == MODEM_FSK )
        {
            SX1278.Settings.FskPacketHandler.PreambleDetected = false;
            SX1278.Settings.FskPacketHandler.SyncWordDetected = false;
            SX1278.Settings.FskPacketHandler.NbBytes = 0;
            SX1278.Settings.FskPacketHandler.Size = 0;

            // Clear Irqs
            SX1278Write( REG_IRQFLAGS1, RF_IRQFLAGS1_RSSI |
                                        RF_IRQFLAGS1_PREAMBLEDETECT |
                                        RF_IRQFLAGS1_SYNCADDRESSMATCH );
            SX1278Write( REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN );

            if( SX1278.Settings.Fsk.RxContinuous == true )
            {
                // Continuous mode restart Rx chain
                SX1278Write( REG_RXCONFIG, SX1278Read( REG_RXCONFIG ) | RF_RXCONFIG_RESTARTRXWITHOUTPLLLOCK );
                TimerStart( &RxTimeoutSyncWord );
            }
            else
            {
                SX1278.Settings.State = RF_IDLE;
                TimerStop( &RxTimeoutSyncWord );
            }
        }
        //debug
        /* SX1278Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_RXTIMEOUT ); */
        /* SX1278.Settings.State = RF_IDLE; */

        if( ( RadioEvents != NULL ) && ( RadioEvents->RxTimeout != NULL ) )
        {
            RadioEvents->RxTimeout( );
        }
        break;
    case RF_TX_RUNNING:
        SX1278.Settings.State = RF_IDLE;
        if( ( RadioEvents != NULL ) && ( RadioEvents->TxTimeout != NULL ) )
        {
            RadioEvents->TxTimeout( );
        }
        break;
    default:
        break;
    }
}

void SX1278OnDio0Irq( void )
{

   /* DEBUG("DIO0 interrupt"); */
    volatile uint8_t irqFlags = 0;

    switch( SX1278.Settings.State )
    {
        case RF_RX_RUNNING:
            /* TimerStop( &RxTimeoutTimer ); */
            // RxDone interrupt
            switch( SX1278.Settings.Modem )
            {
            case MODEM_FSK:
                if( SX1278.Settings.Fsk.CrcOn == true )
                {
                    irqFlags = SX1278Read( REG_IRQFLAGS2 );
                    if( ( irqFlags & RF_IRQFLAGS2_CRCOK ) != RF_IRQFLAGS2_CRCOK )
                    {
                        // Clear Irqs
                        SX1278Write( REG_IRQFLAGS1, RF_IRQFLAGS1_RSSI |
                                                    RF_IRQFLAGS1_PREAMBLEDETECT |
                                                    RF_IRQFLAGS1_SYNCADDRESSMATCH );
                        SX1278Write( REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN );

                        TimerStop( &RxTimeoutTimer );

                        if( SX1278.Settings.Fsk.RxContinuous == false )
                        {
                            TimerStop( &RxTimeoutSyncWord );
                            SX1278.Settings.State = RF_IDLE;
                        }
                        else
                        {
                            // Continuous mode restart Rx chain
                            SX1278Write( REG_RXCONFIG, SX1278Read( REG_RXCONFIG ) | RF_RXCONFIG_RESTARTRXWITHOUTPLLLOCK );
                            TimerStart( &RxTimeoutSyncWord );
                        }

                        if( ( RadioEvents != NULL ) && ( RadioEvents->RxError != NULL ) )
                        {
                            RadioEvents->RxError( );
                        }
                        SX1278.Settings.FskPacketHandler.PreambleDetected = false;
                        SX1278.Settings.FskPacketHandler.SyncWordDetected = false;
                        SX1278.Settings.FskPacketHandler.NbBytes = 0;
                        SX1278.Settings.FskPacketHandler.Size = 0;
                        break;
                    }
                }

                // Read received packet size
                if( ( SX1278.Settings.FskPacketHandler.Size == 0 ) && ( SX1278.Settings.FskPacketHandler.NbBytes == 0 ) )
                {
                    if( SX1278.Settings.Fsk.FixLen == false )
                    {
                        SX1278ReadFifo( ( uint8_t* )&SX1278.Settings.FskPacketHandler.Size, 1 );
                    }
                    else
                    {
                        SX1278.Settings.FskPacketHandler.Size = SX1278Read( REG_PAYLOADLENGTH );
                    }
                    SX1278ReadFifo( RxTxBuffer + SX1278.Settings.FskPacketHandler.NbBytes, SX1278.Settings.FskPacketHandler.Size - SX1278.Settings.FskPacketHandler.NbBytes );
                    SX1278.Settings.FskPacketHandler.NbBytes += ( SX1278.Settings.FskPacketHandler.Size - SX1278.Settings.FskPacketHandler.NbBytes );
                }
                else
                {
                    SX1278ReadFifo( RxTxBuffer + SX1278.Settings.FskPacketHandler.NbBytes, SX1278.Settings.FskPacketHandler.Size - SX1278.Settings.FskPacketHandler.NbBytes );
                    SX1278.Settings.FskPacketHandler.NbBytes += ( SX1278.Settings.FskPacketHandler.Size - SX1278.Settings.FskPacketHandler.NbBytes );
                }

                if( SX1278.Settings.Fsk.RxContinuous == false )
                {
                    SX1278.Settings.State = RF_IDLE;
                    TimerStart( &RxTimeoutSyncWord );
                }
                else
                {
                    // Continuous mode restart Rx chain
                    SX1278Write( REG_RXCONFIG, SX1278Read( REG_RXCONFIG ) | RF_RXCONFIG_RESTARTRXWITHOUTPLLLOCK );
                }
                TimerStop( &RxTimeoutTimer );

                if( ( RadioEvents != NULL ) && ( RadioEvents->RxDone != NULL ) )
                {
                    RadioEvents->RxDone( RxTxBuffer, SX1278.Settings.FskPacketHandler.Size, SX1278.Settings.FskPacketHandler.RssiValue, 0 );
                }
                SX1278.Settings.FskPacketHandler.PreambleDetected = false;
                SX1278.Settings.FskPacketHandler.SyncWordDetected = false;
                SX1278.Settings.FskPacketHandler.NbBytes = 0;
                SX1278.Settings.FskPacketHandler.Size = 0;
                break;
            case MODEM_LORA:
                {
                    int8_t snr = 0;

                    // Clear Irq
                    SX1278Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_RXDONE );

                    irqFlags = SX1278Read( REG_LR_IRQFLAGS );
                    if( ( irqFlags & RFLR_IRQFLAGS_PAYLOADCRCERROR_MASK ) == RFLR_IRQFLAGS_PAYLOADCRCERROR )
                    {
                        // Clear Irq
                        SX1278Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_PAYLOADCRCERROR );

                        if( SX1278.Settings.LoRa.RxContinuous == false )
                        {
                            SX1278.Settings.State = RF_IDLE;
                        }
                        TimerStop( &RxTimeoutTimer );

                        if( ( RadioEvents != NULL ) && ( RadioEvents->RxError != NULL ) )
                        {
                            RadioEvents->RxError( );
                        }
                        break;
                    }

                    //debug
                    /* DEBUG("error frequency = %d",SX1278LoRaGetErrorRFFrequency()); */

                    SX1278.Settings.LoRaPacketHandler.SnrValue = SX1278Read( REG_LR_PKTSNRVALUE );
                    if( SX1278.Settings.LoRaPacketHandler.SnrValue & 0x80 ) // The SNR sign bit is 1
                    {
                        // Invert and divide by 4
                        snr = ( ( ~SX1278.Settings.LoRaPacketHandler.SnrValue + 1 ) & 0xFF ) >> 2;
                        snr = -snr;
                    }
                    else
                    {
                        // Divide by 4
                        snr = ( SX1278.Settings.LoRaPacketHandler.SnrValue & 0xFF ) >> 2;
                    }

                    int16_t rssi = SX1278Read( REG_LR_PKTRSSIVALUE );
                    if( snr < 0 )
                    {
                        if( SX1278.Settings.Channel > RF_MID_BAND_THRESH )
                        {
                            SX1278.Settings.LoRaPacketHandler.RssiValue = RSSI_OFFSET_HF + rssi + ( rssi >> 4 ) +
                                                                          snr;
                        }
                        else
                        {
                            SX1278.Settings.LoRaPacketHandler.RssiValue = RSSI_OFFSET_LF + rssi + ( rssi >> 4 ) +
                                                                          snr;
                        }
                    }
                    else
                    {
                        if( SX1278.Settings.Channel > RF_MID_BAND_THRESH )
                        {
                            SX1278.Settings.LoRaPacketHandler.RssiValue = RSSI_OFFSET_HF + rssi + ( rssi >> 4 );
                        }
                        else
                        {
                            SX1278.Settings.LoRaPacketHandler.RssiValue = RSSI_OFFSET_LF + rssi + ( rssi >> 4 );
                        }
                    }

                    SX1278.Settings.LoRaPacketHandler.Size = SX1278Read( REG_LR_RXNBBYTES );
                    SX1278ReadFifo( RxTxBuffer, SX1278.Settings.LoRaPacketHandler.Size );

                    if( SX1278.Settings.LoRa.RxContinuous == false )
                    {
                        SX1278.Settings.State = RF_IDLE;
                    }
                    TimerStop( &RxTimeoutTimer );

                    if( ( RadioEvents != NULL ) && ( RadioEvents->RxDone != NULL ) )
                    {
                        RadioEvents->RxDone( RxTxBuffer, SX1278.Settings.LoRaPacketHandler.Size, SX1278.Settings.LoRaPacketHandler.RssiValue, SX1278.Settings.LoRaPacketHandler.SnrValue );
                    }
                }
                break;
            default:
                break;
            }
            break;
        case RF_TX_RUNNING:
            TimerStop( &TxTimeoutTimer );
            // TxDone interrupt
            switch( SX1278.Settings.Modem )
            {
            case MODEM_LORA:
                // Clear Irq
                SX1278Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_TXDONE );
                // Intentional fall through
            case MODEM_FSK:
            default:
                SX1278.Settings.State = RF_IDLE;
                if( ( RadioEvents != NULL ) && ( RadioEvents->TxDone != NULL ) )
                {
                    RadioEvents->TxDone( );
                }
                break;
            }
            break;
        default:
            break;
    }
}

void SX1278OnDio1Irq( void )
{
    DEBUG("DIO1 interrupt");
    switch( SX1278.Settings.State )
    {
        case RF_RX_RUNNING:
            switch( SX1278.Settings.Modem )
            {
            case MODEM_FSK:
                // FifoLevel interrupt
                // Read received packet size
                if( ( SX1278.Settings.FskPacketHandler.Size == 0 ) && ( SX1278.Settings.FskPacketHandler.NbBytes == 0 ) )
                {
                    if( SX1278.Settings.Fsk.FixLen == false )
                    {
                        SX1278ReadFifo( ( uint8_t* )&SX1278.Settings.FskPacketHandler.Size, 1 );
                    }
                    else
                    {
                        SX1278.Settings.FskPacketHandler.Size = SX1278Read( REG_PAYLOADLENGTH );
                    }
                }

                if( ( SX1278.Settings.FskPacketHandler.Size - SX1278.Settings.FskPacketHandler.NbBytes ) > SX1278.Settings.FskPacketHandler.FifoThresh )
                {
                    SX1278ReadFifo( ( RxTxBuffer + SX1278.Settings.FskPacketHandler.NbBytes ), SX1278.Settings.FskPacketHandler.FifoThresh );
                    SX1278.Settings.FskPacketHandler.NbBytes += SX1278.Settings.FskPacketHandler.FifoThresh;
                }
                else
                {
                    SX1278ReadFifo( ( RxTxBuffer + SX1278.Settings.FskPacketHandler.NbBytes ), SX1278.Settings.FskPacketHandler.Size - SX1278.Settings.FskPacketHandler.NbBytes );
                    SX1278.Settings.FskPacketHandler.NbBytes += ( SX1278.Settings.FskPacketHandler.Size - SX1278.Settings.FskPacketHandler.NbBytes );
                }
                break;
            case MODEM_LORA:
                // Sync time out
                TimerStop( &RxTimeoutTimer );
                // Clear Irq
                SX1278Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_RXTIMEOUT );

                SX1278.Settings.State = RF_IDLE;
                if( ( RadioEvents != NULL ) && ( RadioEvents->RxTimeout != NULL ) )
                {
                    RadioEvents->RxTimeout( );
                }

                break;
            default:
                break;
            }
            break;
        case RF_TX_RUNNING:
            switch( SX1278.Settings.Modem )
            {
            case MODEM_FSK:
                // FifoEmpty interrupt
                if( ( SX1278.Settings.FskPacketHandler.Size - SX1278.Settings.FskPacketHandler.NbBytes ) > SX1278.Settings.FskPacketHandler.ChunkSize )
                {
                    SX1278WriteFifo( ( RxTxBuffer + SX1278.Settings.FskPacketHandler.NbBytes ), SX1278.Settings.FskPacketHandler.ChunkSize );
                    SX1278.Settings.FskPacketHandler.NbBytes += SX1278.Settings.FskPacketHandler.ChunkSize;
                }
                else
                {
                    // Write the last chunk of data
                    SX1278WriteFifo( RxTxBuffer + SX1278.Settings.FskPacketHandler.NbBytes, SX1278.Settings.FskPacketHandler.Size - SX1278.Settings.FskPacketHandler.NbBytes );
                    SX1278.Settings.FskPacketHandler.NbBytes += SX1278.Settings.FskPacketHandler.Size - SX1278.Settings.FskPacketHandler.NbBytes;
                }
                break;
            case MODEM_LORA:
                break;
            default:
                break;
            }
            break;

        default:
            break;
    }
}

void SX1278OnDio2Irq( void )
{

    DEBUG("DIO2 interrupt");
    switch( SX1278.Settings.State )
    {
        case RF_RX_RUNNING:
            switch( SX1278.Settings.Modem )
            {
            case MODEM_FSK:
                if( ( SX1278.Settings.FskPacketHandler.PreambleDetected == true ) && ( SX1278.Settings.FskPacketHandler.SyncWordDetected == false ) )
                {
                    TimerStop( &RxTimeoutSyncWord );

                    SX1278.Settings.FskPacketHandler.SyncWordDetected = true;

                    SX1278.Settings.FskPacketHandler.RssiValue = -( SX1278Read( REG_RSSIVALUE ) >> 1 );

                    SX1278.Settings.FskPacketHandler.AfcValue = ( int32_t )( double )( ( ( uint16_t )SX1278Read( REG_AFCMSB ) << 8 ) |
                                                                           ( uint16_t )SX1278Read( REG_AFCLSB ) ) *
                                                                           ( double )FREQ_STEP;
                    SX1278.Settings.FskPacketHandler.RxGain = ( SX1278Read( REG_LNA ) >> 5 ) & 0x07;
                }
                break;
            case MODEM_LORA:
                if( SX1278.Settings.LoRa.FreqHopOn == true )
                {
                    // Clear Irq
                    SX1278Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL );

                    if( ( RadioEvents != NULL ) && ( RadioEvents->FhssChangeChannel != NULL ) )
                    {
                        RadioEvents->FhssChangeChannel( ( SX1278Read( REG_LR_HOPCHANNEL ) & RFLR_HOPCHANNEL_CHANNEL_MASK ) );
                    }
                }
                break;
            default:
                break;
            }
            break;
        case RF_TX_RUNNING:
            switch( SX1278.Settings.Modem )
            {
            case MODEM_FSK:
                break;
            case MODEM_LORA:
                if( SX1278.Settings.LoRa.FreqHopOn == true )
                {
                    // Clear Irq
                    SX1278Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL );

                    if( ( RadioEvents != NULL ) && ( RadioEvents->FhssChangeChannel != NULL ) )
                    {
                        RadioEvents->FhssChangeChannel( ( SX1278Read( REG_LR_HOPCHANNEL ) & RFLR_HOPCHANNEL_CHANNEL_MASK ) );
                    }
                }
                break;
            default:
                break;
            }
            break;
        default:
            break;
    }
}

void SX1278OnDio3Irq( void )
{

    DEBUG("DIO3 interrupt");
    switch( SX1278.Settings.Modem )
    {
    case MODEM_FSK:
        break;
    case MODEM_LORA:
        if( ( SX1278Read( REG_LR_IRQFLAGS ) & RFLR_IRQFLAGS_CADDETECTED ) == RFLR_IRQFLAGS_CADDETECTED )
        {
            // Clear Irq
            SX1278Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_CADDETECTED | RFLR_IRQFLAGS_CADDONE );
            if( ( RadioEvents != NULL ) && ( RadioEvents->CadDone != NULL ) )
            {
                RadioEvents->CadDone( true );
            }
        }
        else
        {
            // Clear Irq
            SX1278Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_CADDONE );
            if( ( RadioEvents != NULL ) && ( RadioEvents->CadDone != NULL ) )
            {
                RadioEvents->CadDone( false );
            }
        }
        break;
    default:
        break;
    }
}

void SX1278OnDio4Irq( void )
{
    DEBUG("DIO4 interrupt");
    switch( SX1278.Settings.Modem )
    {
    case MODEM_FSK:
        {
            if( SX1278.Settings.FskPacketHandler.PreambleDetected == false )
            {
                SX1278.Settings.FskPacketHandler.PreambleDetected = true;
            }
        }
        break;
    case MODEM_LORA:
        break;
    default:
        break;
    }
}

void SX1278OnDio5Irq( void )
{
   DEBUG("DIO5 interrupt");
    switch( SX1278.Settings.Modem )
    {
    case MODEM_FSK:
        break;
    case MODEM_LORA:
        break;
    default:
        break;
    }
}


// lz add
uint8_t SX1278GetVersion(void)
{
    return SX1278Read( REG_LR_VERSION );
}

uint32_t SX1278LoRaGetRFFrequency( void )
{
    uint32_t RFFrequency;
    uint8_t freq[3];
    SX1278ReadBuffer( REG_LR_FRFMSB, freq, 3 );
    RFFrequency = ( ( uint32_t )freq[0] << 16 ) | ( ( uint32_t )freq[1] << 8 ) | ( ( uint32_t )freq[2]  );
    RFFrequency = ( uint32_t )( ( double )RFFrequency * ( double )FREQ_STEP );

    return RFFrequency;
}

uint32_t SX1278LoRaGetErrorRFFrequency( void )
{
    uint32_t errorFreq;
    uint8_t freq[3];
    SX1278ReadBuffer( REG_LR_FEIMSB, freq, 3 );
    errorFreq = ( ( uint32_t )freq[0] << 16 ) | ( ( uint32_t )freq[1] << 8 ) | ( ( uint32_t )freq[2]  );
    /* errorFreq = ( uint32_t )( ( double )errorFreq * ( double )FREQ_STEP ); */

    return errorFreq;
}

