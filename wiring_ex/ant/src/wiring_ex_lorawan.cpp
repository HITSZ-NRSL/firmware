
#ifndef WIRING_EX_LORAWAN_H_
#define WIRING_EX_LORAWAN_H_

#include "wiring_ex_lorawan.h"
#include "wiring.h"

<<<<<<< HEAD
#define TEST_SX1278

#ifdef TEST_SX1278

#define USE_BAND_433
#define USE_MODEM_LORA

// #define RF_FREQUENCY                                470600000 // Hz
#define RF_FREQUENCY                                434665000 // Hz

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

=======
#if 0
#define DEFAULT_TIMEOUT     5     // second
#define DEFAULT_TIMEWAIT    100   // millisecond

enum _class_type_t { CLASS_A = 0, CLASS_B, CLASS_C };
enum _device_mode_t { LWABP = 0, LWOTAA, TEST };
enum _band_width_t { BW125 = 125, BW250 = 250, BW500 = 500 };
enum _spreading_factor_t { SF12 = 12, SF11 = 11, SF10 = 10, SF9 = 9, SF8 = 8, SF7 = 7 };
enum _data_rate_t { DR0 = 0, DR1, DR2, DR3, DR4, DR5, DR6, DR7, DR8, DR9, DR10, DR11, DR12, DR13, DR14, DR15 };


/*****************************************************************
Type    DataRate    Configuration   BitRate| TxPower Configuration
EU434   0           SF12/125 kHz    250    | 0       10dBm
        1           SF11/125 kHz    440    | 1       7 dBm
        2           SF10/125 kHz    980    | 2       4 dBm
        3           SF9 /125 kHz    1760   | 3       1 dBm
        4           SF8 /125 kHz    3125   | 4       -2dBm
        5           SF7 /125 kHz    5470   | 5       -5dBm
        6           SF7 /250 kHz    11000  | 6:15    RFU
        7           FSK:50 kbps     50000  |
        8:15        RFU                    |
******************************************************************
Type    DataRate    Configuration   BitRate| TxPower Configuration
EU868   0           SF12/125 kHz    250    | 0       20dBm
        1           SF11/125 kHz    440    | 1       14dBm
        2           SF10/125 kHz    980    | 2       11dBm
        3           SF9 /125 kHz    1760   | 3       8 dBm
        4           SF8 /125 kHz    3125   | 4       5 dBm
        5           SF7 /125 kHz    5470   | 5       2 dBm
        6           SF7 /250 kHz    11000  | 6:15    RFU
        7           FSK:50 kbps     50000  |
        8:15        RFU                    |
******************************************************************
Type    DataRate    Configuration   BitRate| TxPower Configuration
US915   0           SF10/125 kHz    980    | 0       30dBm
        1           SF9 /125 kHz    1760   | 1       28dBm
        2           SF8 /125 kHz    3125   | 2       26dBm
        3           SF7 /125 kHz    5470   | 3       24dBm
        4           SF8 /500 kHz    12500  | 4       22dBm
        5:7         RFU                    | 5       20dBm
        8           SF12/500 kHz    980    | 6       18dBm
        9           SF11/500 kHz    1760   | 7       16dBm
        10          SF10/500 kHz    3900   | 8       14dBm
        11          SF9 /500 kHz    7000   | 9       12dBm
        12          SF8 /500 kHz    12500  | 10      10dBm
        13          SF7 /500 kHz    21900  | 11:15   RFU
        14:15       RFU                    |
*******************************************************************
Type    DataRate    Configuration   BitRate| TxPower Configuration
CN780   0           SF12/125 kHz    250    | 0       10dBm
        1           SF11/125 kHz    440    | 1       7 dBm
        2           SF10/125 kHz    980    | 2       4 dBm
        3           SF9 /125 kHz    1760   | 3       1 dBm
        4           SF8 /125 kHz    3125   | 4       -2dBm
        5           SF7 /125 kHz    5470   | 5       -5dBm
        6           SF7 /250 kHz    11000  | 6:15    RFU
        7           FSK:50 kbps     50000  |
        8:15        RFU                    |
******************************************************************/

class LoRaWanClass
{
    public:
        LoRaWanClass(void);

        void init(void);

        void getId(char *DevAddr, char *DevEUI, char *AppEUI);

        void setDataRate(uint8_t datarate);
        void setADR(bool command);
        void setPower(uint16_t power);
        void setChannelFreq(uint8_t channel, uint32_t freq);
        void setChannelDRRange(uint8_t channel, uint8_t minDR, uint8_t maxDR);
        void setDeciveMode(_device_mode_t mode);
        void setClassType(_class_type_t type);

        bool joinABP(void);
        bool joinOTAA(void);

        bool sendConfirmed(uint8_t *buffer, uint16_t length, uint16_t timeout = DEFAULT_TIMEOUT);
        bool sendUnconfirmed(uint8_t *buffer, uint16_t length, uint16_t timeout = DEFAULT_TIMEOUT);
        uint16_t receivePacket(uint8_t *buffer, uint16_t length, uint16_t *rssi);

        void initP2PMode(uint16_t frequency = 433, _spreading_factor_t spreadingFactor = SF12, _band_width_t bandwidth = BW125,
                         uint8_t txPreamble = 8, uint8_t rxPreamble = 8, uint16_t power = 20);

        bool p2pSend(uint8_t *buffer, uint16_t length);
        uint16_t p2pReceive(uint8_t *buffer, uint16_t length, uint16_t *rssi, uint16_t timeout = DEFAULT_TIMEOUT);

    private:

};

extern LoRaWanClass LoRaWAN;
>>>>>>> develop
#endif

#endif

