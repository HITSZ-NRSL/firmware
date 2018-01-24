/**
 ******************************************************************************
  Copyright (c) 2013-2014 IntoRobot Team.  All right reserved.

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

#include "intorobot_config.h"
#ifdef configSETUP_ENABLE

#include "system_test.h"
#include "wiring_ajson.h"
#include "wiring.h"
#include "system_config.h"
#include "platforms.h"
#include "rtc_hal.h"

#if PLATFORM_ID == PLATFORM_ANT || PLATFORM_ID == PLATFORM_L6
#include "wiring_lorawan.h"
bool TestSX1278Process(int8_t &snr, int8_t &rssi, int8_t &txRssi);
#endif

/*debug switch*/
#define SYSTEM_TEST_DEBUG

#ifdef SYSTEM_TEST_DEBUG
#define STEST_DEBUG(...)    do {DEBUG(__VA_ARGS__);}while(0)
#define STEST_DEBUG_D(...)  do {DEBUG_D(__VA_ARGS__);}while(0)
#define STEST_DEBUG_DUMP    DEBUG_DUMP
#else
#define STEST_DEBUG(...)
#define STEST_DEBUG_D(...)
#define STEST_DEBUG_DUMP
#endif

#ifndef configNO_NETWORK
using namespace intorobot;
#endif

void testDigitalWrite(uint16_t pin, uint16_t value, void* cookie)
{
    aJsonClass aJson;

#if PLATFORM_ID == PLATFORM_ATOM || PLATFORM_ID == PLATFORM_NEUTRON || PLATFORM_ID == PLATFORM_NUT || PLATFORM_ID == PLATFORM_W67
    aJsonObject* root = aJson.createObject();
    char* strPtr = nullptr;

    if(pin == 0xff) {
        uint8_t pinNumber;
        for(pinNumber = 1; pinNumber <= TOTAL_DIGITAL_PINS; pinNumber++) {
            pinMode(pinNumber,OUTPUT);
            digitalWrite(pinNumber,value);
        }
    } else {
        pinMode(pin,OUTPUT);
        digitalWrite(pin,value);
    }
    aJson.addNumberToObject(root, "status", 200);
    strPtr = aJson.print(root);
    ((DeviceConfig*)cookie)->write((unsigned char *)strPtr, strlen(strPtr));
    free(strPtr);
    aJson.deleteItem(root);

#elif PLATFORM_ID == PLATFORM_FIG
    aJsonObject* root = aJson.createObject();
    char* strPtr = nullptr;

    if(pin == 0xff) {
        uint8_t pinNumber;
        for(pinNumber = D0; pinNumber <= D6; pinNumber++) {
            pinMode(pinNumber,OUTPUT);
            digitalWrite(pinNumber,value);
        }
        for(pinNumber = A3; pinNumber <= A9; pinNumber++) {
            pinMode(pinNumber,OUTPUT);
            digitalWrite(pinNumber,value);
        }
    } else {
        pinMode(pin,OUTPUT);
        digitalWrite(pin,value);
    }
    aJson.addNumberToObject(root, "status", 200);
    strPtr = aJson.print(root);
    ((DeviceConfig*)cookie)->write((unsigned char *)strPtr, strlen(strPtr));
    free(strPtr);
    aJson.deleteItem(root);

#elif PLATFORM_ID == PLATFORM_W323
    aJsonObject* root = aJson.createObject();
    char* strPtr = nullptr;
    uint8_t pinNumber;

    if(pin == 0xff) {
        uint8_t pinNumber;
        for(pinNumber = 2; pinNumber <= 9; pinNumber++) {
            pinMode(pinNumber,OUTPUT);
            digitalWrite(pinNumber,value);
        }
        for(pinNumber = 30; pinNumber <= 41; pinNumber++) {
            pinMode(pinNumber,OUTPUT);
            digitalWrite(pinNumber,value);
        }

    } else {
        pinMode(pin,OUTPUT);
        digitalWrite(pin,value);
    }
    aJson.addNumberToObject(root, "status", 200);
    strPtr = aJson.print(root);
    ((DeviceConfig*)cookie)->write((unsigned char *)strPtr, strlen(strPtr));
    free(strPtr);
    aJson.deleteItem(root);

#elif PLATFORM_ID == PLATFORM_ANT
    aJsonObject* root = aJson.createObject();
    char* strPtr = nullptr;
    if(pin == 0xff) {
        uint8_t pinNumber;
        for(pinNumber = D0; pinNumber <= D4; pinNumber++) {
            pinMode(pinNumber,OUTPUT);
            digitalWrite(pinNumber,value);
        }
        for(pinNumber = A0; pinNumber <= A5; pinNumber++) {
            pinMode(pinNumber,OUTPUT);
            digitalWrite(pinNumber,value);
        }
    } else {
        pinMode(pin,OUTPUT);
        digitalWrite(pin,value);
    }

    aJson.addNumberToObject(root, "status", 200);
    strPtr = aJson.print(root);
    ((DeviceConfig*)cookie)->write((unsigned char *)strPtr, strlen(strPtr));
    free(strPtr);
    aJson.deleteItem(root);

#elif PLATFORM_ID == PLATFORM_L6
    aJsonObject* root = aJson.createObject();
    char* strPtr = nullptr;
    if(pin == 0xff) {
        uint8_t pinNumber;
        for(pinNumber = 0; pinNumber < 9; pinNumber++) {
            pinMode(pinNumber,OUTPUT);
            digitalWrite(pinNumber,value);
        }
        for(pinNumber = 11; pinNumber < 19; pinNumber++) {
            pinMode(pinNumber,OUTPUT);
            digitalWrite(pinNumber,value);
        }
    } else {
        pinMode(pin,OUTPUT);
        digitalWrite(pin,value);
    }

    aJson.addNumberToObject(root, "status", 200);
    strPtr = aJson.print(root);
    ((DeviceConfig*)cookie)->write((unsigned char *)strPtr, strlen(strPtr));
    free(strPtr);
    aJson.deleteItem(root);

#elif PLATFORM_ID == PLATFORM_FOX
    aJsonObject* root = aJson.createObject();
    char* strPtr = nullptr;
    if(pin == 0xff) {
        uint8_t pinNumber;
        for(pinNumber = 0; pinNumber < 5; pinNumber++) {
            pinMode(pinNumber,OUTPUT);
            digitalWrite(pinNumber,value);
        }
        pinMode(7,OUTPUT);
        digitalWrite(7,value);
        for(pinNumber = 30; pinNumber < 37; pinNumber++) {
            pinMode(pinNumber,OUTPUT);
            digitalWrite(pinNumber,value);
        }
    } else {
        pinMode(pin,OUTPUT);
        digitalWrite(pin,value);
    }

    aJson.addNumberToObject(root, "status", 200);
    strPtr = aJson.print(root);
    ((DeviceConfig*)cookie)->write((unsigned char *)strPtr, strlen(strPtr));
    free(strPtr);
    aJson.deleteItem(root);
#endif
}

void testAnalogRead(uint16_t pin, void* cookie)
{
    aJsonClass aJson;

#if PLATFORM_ID == PLATFORM_ATOM || PLATFORM_ID == PLATFORM_NEUTRON || PLATFORM_ID == PLATFORM_W67 || PLATFORM_ID == PLATFORM_NUT || PLATFORM_ID == PLATFORM_FIG || PLATFORM_ID == PLATFORM_W323
    aJsonObject* root = aJson.createObject();
    char* strPtr = nullptr;
    aJson.addNumberToObject(root, "status", 200);
    aJson.addNumberToObject(root, "value", (int)analogRead(pin));
    strPtr = aJson.print(root);
    ((DeviceConfig*)cookie)->write((unsigned char *)strPtr, strlen(strPtr));
    free(strPtr);
    aJson.deleteItem(root);

#elif PLATFORM_ID == PLATFORM_ANT
    aJsonObject* root = aJson.createObject();
    char* strPtr = nullptr;
    aJson.addNumberToObject(root, "status", 200);
    aJson.addNumberToObject(root, "value", (int)analogRead(pin));
    strPtr = aJson.print(root);
    ((DeviceConfig*)cookie)->write((unsigned char *)strPtr, strlen(strPtr));
    free(strPtr);
    aJson.deleteItem(root);

#elif PLATFORM_ID == PLATFORM_L6
    aJsonObject* root = aJson.createObject();
    char* strPtr = nullptr;
    aJson.addNumberToObject(root, "status", 200);
    aJson.addNumberToObject(root, "value", (int)analogRead(pin));
    strPtr = aJson.print(root);
    ((DeviceConfig*)cookie)->write((unsigned char *)strPtr, strlen(strPtr));
    free(strPtr);
    aJson.deleteItem(root);

#elif PLATFORM_ID == PLATFORM_FOX
    aJsonObject* root = aJson.createObject();
    char* strPtr = nullptr;
    aJson.addNumberToObject(root, "status", 200);
    aJson.addNumberToObject(root, "value", (int)analogRead(pin));
    strPtr = aJson.print(root);
    ((DeviceConfig*)cookie)->write((unsigned char *)strPtr, strlen(strPtr));
    free(strPtr);
    aJson.deleteItem(root);

#endif
}

void testSelfTest(void* cookie)
{
    aJsonClass aJson;

#if PLATFORM_ID == PLATFORM_ATOM || PLATFORM_ID == PLATFORM_NEUTRON || PLATFORM_ID == PLATFORM_W67 || PLATFORM_ID == PLATFORM_NUT || PLATFORM_ID == PLATFORM_FIG || PLATFORM_ID == PLATFORM_W323
    aJsonObject* root = aJson.createObject();
    char* strPtr = nullptr;
    aJson.addNumberToObject(root, "status", 200);
    strPtr = aJson.print(root);
    ((DeviceConfig*)cookie)->write((unsigned char *)strPtr, strlen(strPtr));
    aJson.deleteItem(root);
    free(strPtr);

#elif PLATFORM_ID == PLATFORM_ANT
    aJsonObject* root = aJson.createObject();
    char* strPtr = nullptr;
    if(HAL_RTC_Time_Is_Valid(nullptr)) {
        aJson.addNumberToObject(root, "status", 200);
    } else {
        aJson.addNumberToObject(root, "status", 201);
    }
    strPtr = aJson.print(root);
    ((DeviceConfig*)cookie)->write((unsigned char *)strPtr, strlen(strPtr));
    free(strPtr);
    aJson.deleteItem(root);

#elif PLATFORM_ID == PLATFORM_L6
    aJsonObject* root = aJson.createObject();
    char* strPtr = nullptr;
    if(HAL_RTC_Time_Is_Valid(nullptr)) {
        aJson.addNumberToObject(root, "status", 200);
    } else {
        aJson.addNumberToObject(root, "status", 201);
    }
    strPtr = aJson.print(root);
    ((DeviceConfig*)cookie)->write((unsigned char *)strPtr, strlen(strPtr));
    free(strPtr);
    aJson.deleteItem(root);

#elif PLATFORM_ID == PLATFORM_FOX
    aJsonObject* root = aJson.createObject();
    char* strPtr = nullptr;
    if(HAL_RTC_Time_Is_Valid(nullptr)) {
        aJson.addNumberToObject(root, "status", 200);
    } else {
        aJson.addNumberToObject(root, "status", 201);
    }
    strPtr = aJson.print(root);
    ((DeviceConfig*)cookie)->write((unsigned char *)strPtr, strlen(strPtr));
    free(strPtr);
    aJson.deleteItem(root);

#endif
}

void testRfCheck(void* cookie)
{
#if PLATFORM_ID == PLATFORM_ATOM || PLATFORM_ID == PLATFORM_NEUTRON || PLATFORM_ID == PLATFORM_NUT || PLATFORM_ID == PLATFORM_W67 || PLATFORM_ID == PLATFORM_FIG || PLATFORM_ID == PLATFORM_W323
    ((DeviceConfig*)cookie)->dealGetWifiList();

#elif (PLATFORM_ID == PLATFORM_ANT) || (PLATFORM_ID == PLATFORM_L6)
    aJsonClass aJson;
    aJsonObject* root = aJson.createObject();
    char* strPtr = nullptr;
    int8_t snrVal = 0;
    int8_t rssiVal = 0;
    int8_t txRssiVal = 0;

    if(TestSX1278Process(snrVal,rssiVal,txRssiVal)) {
        aJson.addNumberToObject(root, "status", 200);
    } else {
        aJson.addNumberToObject(root, "status", 201);
    }

    aJson.addNumberToObject(root, "snr", snrVal);
    aJson.addNumberToObject(root, "rssi", rssiVal);
    aJson.addNumberToObject(root, "txrssi", txRssiVal);

    strPtr = aJson.print(root);
    ((DeviceConfig*)cookie)->write((unsigned char *)strPtr, strlen(strPtr));
    free(strPtr);
    aJson.deleteItem(root);

#elif (PLATFORM_ID == PLATFORM_FOX)
    aJsonClass aJson;
    aJsonObject* root = aJson.createObject();
    char* strPtr = nullptr;

    CellularSignal sig;
    sig = Cellular.RSSI();

    aJson.addNumberToObject(root, "status", 200);
    aJson.addNumberToObject(root, "rssi", sig.rssi);

    strPtr = aJson.print(root);
    ((DeviceConfig*)cookie)->write((unsigned char *)strPtr, strlen(strPtr));
    free(strPtr);
    aJson.deleteItem(root);

#endif
}

void testSensorData(void* cookie)
{
    ((DeviceConfig*)cookie)->sendComfirm(200);
}

#if PLATFORM_ID == PLATFORM_ANT || PLATFORM_ID == PLATFORM_L6

//ant l6生产时板子测试代码
#define RF_FREQUENCY    433975000
#define BUFFER_SIZE     8

typedef enum
{
    IDLE,
    TX_START,
    RX_DONE,
    RX_TIMEOUT,
    RX_ERROR,
    TX_DONE,
    TX_TIMEOUT,
}run_state_t;

static int8_t buffer[BUFFER_SIZE] = {11,2,3,4,5,6,7,8};
static run_state_t runState = TX_START;
static uint8_t sx1278Version = 0;

static void TestLoRaRadioEventCallback(system_event_t event, int param, uint8_t *data, uint16_t datalen)
{
    if(event == event_lora_radio_status)
    {
        switch(param)
        {
        case ep_lora_radio_tx_done:
            LoRa.radioSetSleep();
            runState = TX_DONE;
            break;

        case ep_lora_radio_tx_fail:
            LoRa.radioSetSleep();
            runState = TX_TIMEOUT;
            break;

        case ep_lora_radio_rx_done:
            LoRa.radioSetSleep();
            runState = RX_DONE;
            memcpy( buffer, data, datalen );
            #if  0
            DEBUG("rx done\r\n");
            Serial.printf("datalen = %d\r\n",datalen);
            for(uint8_t i=0; i<datalen;i++)
            {
                Serial.printf("%d ",buffer[i]);
            }
            Serial.print("\r\n");
            Serial.printf("rssi = %d\r\n",LoRa.radioGetRssi());
            #endif
            break;

        case ep_lora_radio_rx_timeout:
            LoRa.radioSetSleep();
            runState = RX_TIMEOUT;
            break;

        case ep_lora_radio_rx_error:
            LoRa.radioSetSleep();
            runState = RX_ERROR;
            break;
        default:
            break;
        }
    }
}

void TestSX1278Init()
{
    SX1276BoardInit();
    Cloud.setProtocol(PROTOCOL_P2P);
    System.on(event_lora_radio_status, &TestLoRaRadioEventCallback);
    LoRa.radioSetFreq(RF_FREQUENCY);
    LoRa.radioSetIqInverted(true);
    sx1278Version = LoRa.radioReadReg(0x42);
}

bool TestSX1278Process(int8_t &snr, int8_t &rssi, int8_t &txRssi)
{
    TestSX1278Init();
    if(sx1278Version != 0x12){
        return false;
    }
    while(1){
        switch(runState){
        case TX_START:
            runState = IDLE;
            LoRa.radioSend(buffer, BUFFER_SIZE, 3000);
            break;

        case TX_DONE:
            runState = IDLE;
            LoRa.radioStartRx(2000);
            break;

        case RX_DONE:
            runState = TX_START;
            if(buffer[0] == 22){
                snr = LoRa.radioGetSnr();
                rssi = LoRa.radioGetRssi();
                txRssi = buffer[1];
                // DEBUG("snr = %d\r\n",snr);
                // DEBUG("rssi = %d\r\n",rssi);
                // DEBUG("txRssi = %d\r\n",txRssi);
                return true;
            }else{
                return false;
            }
            break;

        case TX_TIMEOUT:
        case RX_TIMEOUT:
        case RX_ERROR:
            return false;
            break;
        case IDLE:
            break;
        default:
            break;
        }

        intorobot_process();
    }
}
#endif
#endif
