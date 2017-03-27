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

#include "ajson.h"
#include "wiring.h"
#include "system_test.h"
#include "system_config.h"
#include "platforms.h"

/*debug switch*/
#define SYSTEM_TEST_DEBUG

#ifdef SYSTEM_TEST_DEBUG
#define STEST_DEBUG(...)  do {DEBUG(__VA_ARGS__);}while(0)
#define STEST_DEBUG_D(...)  do {DEBUG_D(__VA_ARGS__);}while(0)
#else
#define STEST_DEBUG(...)
#define STEST_DEBUG_D(...)
#endif

#ifndef configNO_NETWORK
using namespace intorobot;
#endif

void testDigitalWrite(uint16_t pin, uint16_t value, void* cookie)
{
    aJsonClass aJson;

#if PLATFORM_ID == PLATFORM_ATOM

#elif PLATFORM_ID == PLATFORM_NEUTRON

#elif PLATFORM_ID == PLATFORM_NUT

#elif PLATFORM_ID == PLATFORM_W67
    aJsonObject* root = aJson.createObject();
    char* strPtr = nullptr;

    if(pin == 0xff)
    {
        uint8_t pinNumber;
        for(pinNumber = 1; pinNumber <= TOTAL_DIGITAL_PINS; pinNumber++)
        {
            pinMode(pinNumber,OUTPUT);
            digitalWrite(pinNumber,value);
        }
    }
    else
    {
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

    if(pin == 0xff)
    {
        uint8_t pinNumber;
        for(pinNumber = D0; pinNumber <= D6; pinNumber++)
        {
            pinMode(pinNumber,OUTPUT);
            digitalWrite(pinNumber,value);
        }

        for(pinNumber = A3; pinNumber <= A9; pinNumber++)
        {
            pinMode(pinNumber,OUTPUT);
            digitalWrite(pinNumber,value);
        }

    }
    else
    {
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

    if(pin == 0xff)
    {
        uint8_t pinNumber;
        pinMode(0,OUTPUT);
        pinMode(2,OUTPUT);
        digitalWrite(0,value);
        digitalWrite(2,value);
        for(pinNumber = 4; pinNumber <= 35; pinNumber++)
        {
            pinMode(pinNumber,OUTPUT);
            digitalWrite(pinNumber,value);
        }
    }
    else
    {
        pinMode(pin,OUTPUT);
        digitalWrite(pin,value);
    }
    aJson.addNumberToObject(root, "status", 200);
    strPtr = aJson.print(root);
    ((DeviceConfig*)cookie)->write((unsigned char *)strPtr, strlen(strPtr));
    free(strPtr);
    aJson.deleteItem(root);


#elif PLATFORM_ID == PLATFORM_ANT

#elif PLATFORM_ID == PLATFORM_L6

#elif PLATFORM_ID == PLATFORM_FOX
#endif
}

void testAnalogRead(uint16_t pin, void* cookie)
{
    aJsonClass aJson;

#if PLATFORM_ID == PLATFORM_ATOM

#elif PLATFORM_ID == PLATFORM_NEUTRON

#elif PLATFORM_ID == PLATFORM_W67 || PLATFORM_ID == PLATFORM_NUT
    aJsonObject* root = aJson.createObject();
    char* strPtr = nullptr;
    aJson.addNumberToObject(root, "status", 200);
    aJson.addNumberToObject(root, "value", analogRead(pin));
    strPtr = aJson.print(root);
    ((DeviceConfig*)cookie)->write((unsigned char *)strPtr, strlen(strPtr));
    free(strPtr);
    aJson.deleteItem(root);

#elif PLATFORM_ID == PLATFORM_FIG

#elif PLATFORM_ID == PLATFORM_W323

#elif PLATFORM_ID == PLATFORM_ANT

#elif PLATFORM_ID == PLATFORM_L6

#elif PLATFORM_ID == PLATFORM_FOX

#endif
}

void testSelfTest(void* cookie)
{
    aJsonClass aJson;
#if PLATFORM_ID == PLATFORM_ATOM

#elif PLATFORM_ID == PLATFORM_NEUTRON

#elif PLATFORM_ID == PLATFORM_W67 || PLATFORM_ID == PLATFORM_NUT
    aJsonObject* root = aJson.createObject();
    char* strPtr = nullptr;
    aJson.addNumberToObject(root, "status", 200);
    strPtr = aJson.print(root);
    ((DeviceConfig*)cookie)->write((unsigned char *)strPtr, strlen(strPtr));
    aJson.deleteItem(root);
    free(strPtr);

#elif PLATFORM_ID == PLATFORM_FIG

#elif PLATFORM_ID == PLATFORM_W323

#elif PLATFORM_ID == PLATFORM_ANT

#elif PLATFORM_ID == PLATFORM_L6

#elif PLATFORM_ID == PLATFORM_FOX

#endif
}

void testRfCheck(void* cookie)
{
#if PLATFORM_ID == PLATFORM_ATOM || PLATFORM_ID == PLATFORM_NEUTRON || PLATFORM_ID == PLATFORM_NUT || PLATFORM_ID == PLATFORM_W67 || PLATFORM_ID == PLATFORM_FIG || PLATFORM_ID == PLATFORM_W323

    ((DeviceConfig*)cookie)->dealGetWifiList();

#elif defined(TEST_LORA) || defined(TEST_L6)
#elif PLATFORM_ID == PLATFORM_W67

#endif
}

void testSensorData(void* cookie)
{
}

#endif
