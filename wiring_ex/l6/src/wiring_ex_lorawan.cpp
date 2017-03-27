#include "wiring_ex_lorawan.h"
#include "wiring.h"

#define TEST_SX1278

#ifdef TEST_SX1278

#define USE_BAND_433
#define USE_MODEM_LORA

#define RF_FREQUENCY                                470600000 // Hz

#define TX_OUTPUT_POWER                            20        // dBm

#define LORA_BANDWIDTH                             0         // [0: 125 kHz,
                                                             //  1: 250 kHz,
                                                             //  2: 500 kHz,
                                                             //  3: Reserved]

#define LORA_SPREADING_FACTOR                      7         // [SF7..SF12]
#define LORA_CODINGRATE                            1         // [1: 4/5,
                                                             //  2: 4/6,
                                                             //  3: 4/7,
                                                             //  4: 4/8]

#define LORA_PREAMBLE_LENGTH                       8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                        1023      // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                 false
#define LORA_IQ_INVERSION_ON                       true


typedef enum
{
    LOWPOWER,
    RX_SX1278,
    RX_TIMEOUT,
    RX_ERROR,
    TX_SX1278,
    TX_TIMEOUT,
    TX_STATUS,
    RX_STATUS
}States_t;

#define RX_TIMEOUT_VALUE    1000
#define BUFFER_SIZE         64 // Define the payload size here

uint16_t BufferSize = BUFFER_SIZE;

States_t State = LOWPOWER;

int8_t RssiValue = 0;
int8_t SnrValue = 0;
uint32_t currentTime = 0;
uint8_t sx1278Version = 0;

int8_t Buffer[BUFFER_SIZE] = {11,20,30,40,50,60,70,80};

static RadioEvents_t RadioEvents;
void OnTxDone( void );
void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );
void OnTxTimeout( void );
void OnRxTimeout( void );
void OnRxError( void );
void TestSX178Init(void);
uint8_t ProcessSX1278(void);

void TestSX178Init(void)
{
    delay(100);

    SX1278BoardInit();

    // Radio initialization
    RadioEvents.TxDone = OnTxDone;
    RadioEvents.RxDone = OnRxDone;
    RadioEvents.TxTimeout = OnTxTimeout;
    RadioEvents.RxTimeout = OnRxTimeout;
    RadioEvents.RxError = OnRxError;

    Radio.Init( &RadioEvents );

    Radio.SetChannel( RF_FREQUENCY );

    Radio.SetModem( MODEM_LORA );

    sx1278Version = SX1278GetVersion();
    DEBUG("sx1278 version = %d", sx1278Version);
    DEBUG("sx1278 freq = %d",SX1278LoRaGetRFFrequency());

    Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                       LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                       LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                       true, 0, 0, LORA_IQ_INVERSION_ON, 3000 );

    Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                       LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                       LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                       0, true, 0, 0, LORA_IQ_INVERSION_ON, 1 );

    DEBUG("sync data = 0x%x",SX1278Read(0x39));

    Radio.Rx( 0);

    State = TX_STATUS;
    currentTime = timerGetId();
}


uint8_t ProcessSX1278(void)
{
    switch(State)
    {
        case TX_STATUS:
                Radio.Send( Buffer, BufferSize );
                State = LOWPOWER;
                return 0;
            break;

        case TX_SX1278:
                Radio.Rx( 0 );
                State = LOWPOWER;
                return 1;
            break;

        case RX_SX1278:
            return 2;
            break;

        case LOWPOWER:
            if(timerIsEnd(currentTime,2000)) // 2s rx timeout
            {
                return 255;
            }
            else
            {
                return 1;
            }

            break;

        default:
            break;
    }
}

void OnTxDone( void )
{
    Radio.Sleep( );
    State = TX_SX1278;
    DEBUG("tx done");
}

void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    Radio.Sleep( );
    BufferSize = size;
    memcpy( Buffer, payload, BufferSize );
    State = RX_SX1278;

    RssiValue = rssi;
    SnrValue = snr;

    for(uint8_t i=0; i<64; i++)
    {
        if(Buffer[i] != 0)
        {
            DEBUG("number = %d",i);
            DEBUG("val = %d",Buffer[i]);
        }
    }
}

void OnTxTimeout( void )
{
    Radio.Sleep( );
    State = TX_TIMEOUT;
}

void OnRxTimeout( void )
{
    Radio.Sleep( );
    State = RX_TIMEOUT;
}

void OnRxError( void )
{
    Radio.Sleep( );
    State = RX_ERROR;
}


uint32_t currTime = 0;

bool SX1278Test(int8_t &snr, int8_t &rssi, int8_t &txRssi)
{
    TestSX178Init();
    if(sx1278Version != 18)
    {
        return false;
    }
    else
    {
        currTime = timerGetId();
        while(1)
        {
            switch(ProcessSX1278())
            {
                case 0:
                    if(timerIsEnd(currTime,2000)) // tx timeout
                    {
                        return false;
                    }
                    break;

                case 1:
                    break;

                case 2:
                    if(Buffer[0] == 22)
                    {
                        snr = SnrValue;
                        rssi = RssiValue;
                        txRssi = Buffer[1];
                        DEBUG("snr = %d",SnrValue);
                        DEBUG("rssi = %d",RssiValue);
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                    break;

                default:
                    return false; // rx timeout
                    break;
            }
        }
    }
}

#else

bool SX1278Test(int8_t &snr, int8_t &rssi, int8_t &txRssi)
{
    return false;
}

#endif
