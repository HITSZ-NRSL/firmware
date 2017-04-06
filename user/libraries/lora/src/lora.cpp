
#include "lora.h"
//L7 测试板程序
#include "platforms.h"

#if PLATFORM_ID == PLATFORM_ANYTEST

#include "application.h"

// Serial1DebugOutput debugOutput(115200, ALL_LEVEL);

#define USE_BAND_433
#define USE_MODEM_LORA

#define RF_FREQUENCY                                470000000 // Hz

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

const uint8_t PingMsg[] = "PING";
const uint8_t PongMsg[] = "PONG";

uint16_t BufferSize = BUFFER_SIZE;

States_t State = LOWPOWER;

int8_t RssiValue = 0;
int8_t SnrValue = 0;

/*!
 * Radio events function pointer
 */
static RadioEvents_t RadioEvents;

/*!
 * \brief Function to be executed on Radio Tx Done event
 */
void OnTxDone( void );

/*!
 * \brief Function to be executed on Radio Rx Done event
 */
void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );

/*!
 * \brief Function executed on Radio Tx Timeout event
 */
void OnTxTimeout( void );

/*!
 * \brief Function executed on Radio Rx Timeout event
 */
void OnRxTimeout( void );

/*!
 * \brief Function executed on Radio Rx Error event
 */
void OnRxError( void );


TimerEvent_t RxWindowTimer1;

#if 0
void OnRxWindow1TimerEvent()
{
    TimerStop( &RxWindowTimer1 );

    if( Radio.GetStatus( ) == RF_IDLE )
        {
            Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                               LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                               LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                               0, true, 0, 0, LORA_IQ_INVERSION_ON, 0 );

            Radio.Rx( 3000 );
            // delay(1);
            // for(uint8_t i=1; i<0x4f;i++)
            //     {
            //         DEBUG("reg address = %x",i);
            //         DEBUG("reg dat = %x",SX1278Read(i));
            //     }
            DEBUG("startup rx");
        }
}
#endif

#define LED           D13
#define RED_LED       D10
#define GRREN_LED     D11
#define BLUE_LED      D12

static uint32_t currentTime;

void TestSX178Init(void)
{
    pinMode(LED,OUTPUT);
    pinMode(RED_LED,OUTPUT);
    pinMode(GRREN_LED,OUTPUT);
    pinMode(BLUE_LED,OUTPUT);
    digitalWrite(RED_LED,1);
    digitalWrite(GRREN_LED,1);
    digitalWrite(BLUE_LED,1);
    // Serial1.begin(115200);

    delay(100);
    // DEBUG("***********SX1278 Lora test******************");
    Serial1.printf("system clock = %d",SystemCoreClock);
    Serial1.print("\r\n");

    // Target board initialisation
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
    // Change LoRa modem SyncWord
    // Radio.Write( REG_LR_SYNCWORD, 0x34 );

    Serial1.printf("sx1278 version = %d", SX1278GetVersion());
    Serial1.print("\r\n");
    Serial1.printf("sx1278 freq = %d",SX1278LoRaGetRFFrequency());
    Serial1.print("\r\n");

    Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                       LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                       LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                       true, 0, 0, LORA_IQ_INVERSION_ON, 3000 );

    Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                       LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                       LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                       0, true, 0, 0, LORA_IQ_INVERSION_ON, 1 );

    // TimerInit( &RxWindowTimer1, OnRxWindow1TimerEvent );
    // TimerSetValue( &RxWindowTimer1, 1000 ); // 1s后调用OnRxWindow1TimerEvent
    // TimerStart( &RxWindowTimer1 );

    // Radio.Rx( RX_TIMEOUT_VALUE );
    Serial1.printf("sync data = 0x%x",SX1278Read(0x39));
    Serial1.print("\r\n");
    Serial1.println("sx1278 init success ");
    Serial1.println("*********************************************");

    Radio.Rx( 0);

    State = TX_STATUS;
    currentTime = timerGetId();
}


int8_t Buffer[BUFFER_SIZE] = {11,20,30,40,50,60,70,80};



uint8_t ProcessSX1278(int8_t &rssi, int8_t &snr, int8_t *dat)
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
            for(uint8_t i=0; i<64;i++)
            {
                dat[i] = Buffer[i];
            }
            rssi = RssiValue;
            snr = SnrValue;
            return 2;
            break;

        case LOWPOWER:
            if(timerIsEnd(currentTime,2000)) // 5s timeout
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
    Serial1.println("tx done");
}

void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    Radio.Sleep( );
    BufferSize = size;
    memcpy( Buffer, payload, BufferSize );
    State = RX_SX1278;

    RssiValue = rssi;
    // DEBUG("rssi = %d",RssiValue);
    SnrValue = snr;
    // DEBUG("snr = %d",SnrValue);

    for(uint8_t i=0; i<64; i++)
    {
        if(Buffer[i] != 0)
        {
            Serial1.printf("number = %d",i);
            Serial1.print("\r\n");
            Serial1.printf("val = %d",Buffer[i]);
            Serial1.print("\r\n");
        }
    }

    // TimerStart( &RxWindowTimer1 );
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

    // DEBUG("rx timeout");
    // DEBUG("radio status = %d",Radio.GetStatus( ) );
    // TimerStart( &RxWindowTimer1 );
}

void OnRxError( void )
{
    Radio.Sleep( );
    State = RX_ERROR;
    // DEBUG("rx error");
    // TimerStart( &RxWindowTimer1 );
}

#endif
