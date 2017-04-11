
#ifndef WIRING_EX_LORAWAN_H_
#define WIRING_EX_LORAWAN_H_

#include "wiring_ex_lorawan.h"
#include "wiring.h"

bool SX1278Test(int8_t &snrVal, int8_t &rssiVal,int8_t &txRssiVal)
{
    return false;
}


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
#endif

#endif
