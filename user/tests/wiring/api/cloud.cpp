/**
 ******************************************************************************
 * @file    cloud.cpp
 * @authors Matthew McGowan
 * @date    21 January 2015
 ******************************************************************************
  Copyright (c) 2015 IntoRobot Industries, Inc.  All rights reserved.

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

#include "testapi.h"

#ifndef configNO_CLOUD

test(api_intorobot_publish) {
    bool valueBool = 0;
    int valueInt = 0;
    int8_t valueInt8 = 0;
    uint8_t valueUint8 = 0;
    int16_t valueInt16 = 0;
    uint16_t valueUint16 = 0;
    int32_t valueInt32 = 0;
    uint32_t valueUint32 = 0;
    float valueFloat = 0;
    double valueDouble = 0;
    char valueString[] = "oh yeah!";
    String valueSmartString(valueString);

    API_COMPILE(IntoRobot.publish("publish topic name", false));
    API_COMPILE(IntoRobot.publish("publish topic name", 323));
    API_COMPILE(IntoRobot.publish("publish topic name", 323.32));
    API_COMPILE(IntoRobot.publish("publish topic name", "oh yeah!"));

    API_COMPILE(IntoRobot.publish("publish topic name", valueBool));
    API_COMPILE(IntoRobot.publish("publish topic name", valueInt));
    API_COMPILE(IntoRobot.publish("publish topic name", valueInt8));
    API_COMPILE(IntoRobot.publish("publish topic name", valueUint8));
    API_COMPILE(IntoRobot.publish("publish topic name", valueInt16));
    API_COMPILE(IntoRobot.publish("publish topic name", valueUint16));
    API_COMPILE(IntoRobot.publish("publish topic name", valueInt32));
    API_COMPILE(IntoRobot.publish("publish topic name", valueUint32));
    API_COMPILE(IntoRobot.publish("publish topic name", valueFloat));
    API_COMPILE(IntoRobot.publish("publish topic name", valueDouble));
    API_COMPILE(IntoRobot.publish("publish topic name", valueSmartString));
    API_COMPILE(IntoRobot.publish("publish topic name", valueString));
    API_COMPILE(IntoRobot.publish("publish topic name", valueString, strlen(valueString)));
    API_COMPILE(IntoRobot.publish("publish topic name", valueString, strlen(valueString), 0));
    API_COMPILE(IntoRobot.publish("publish topic name", valueString, strlen(valueString), 0, 1));
}

test(api_intorobot_subscribe) {
    void (*handler)(uint8_t*, uint32_t) = NULL;
    WidgetBaseClass WidgetBase;

    API_COMPILE(IntoRobot.subscribe("subscribe topic name", NULL, handler));
    API_COMPILE(IntoRobot.subscribe("subscribe topic name", NULL, handler, 0));
    API_COMPILE(IntoRobot.subscribe("subscribe topic name", NULL, &WidgetBase));
    API_COMPILE(IntoRobot.subscribe("subscribe topic name", NULL, &WidgetBase, 0));
}

test(api_intorobot_unsubscribe) {
    API_COMPILE(IntoRobot.unsubscribe("unsubscribe topic name", NULL));
}

test(api_intorobot_defineDatapoint) {
    API_COMPILE(IntoRobot.defineDatapointBool(1, DP_PERMISSION_UP_ONLY));
    API_COMPILE(IntoRobot.defineDatapointBool(1, DP_PERMISSION_UP_ONLY, false));
    API_COMPILE(IntoRobot.defineDatapointBool(1, DP_PERMISSION_UP_ONLY, false, DP_POLICY_TIMED));
    API_COMPILE(IntoRobot.defineDatapointBool(1, DP_PERMISSION_UP_ONLY, false, DP_POLICY_TIMED, 0));

    API_COMPILE(IntoRobot.defineDatapointNumber(1,DP_PERMISSION_UP_ONLY, -100, 100, 1));
    API_COMPILE(IntoRobot.defineDatapointNumber(1,DP_PERMISSION_UP_ONLY, -100, 100, 1, 0));
    API_COMPILE(IntoRobot.defineDatapointNumber(1,DP_PERMISSION_UP_ONLY, -100, 100, 1, 0, DP_POLICY_TIMED));
    API_COMPILE(IntoRobot.defineDatapointNumber(1,DP_PERMISSION_UP_ONLY, -100, 100, 1, 0, DP_POLICY_TIMED, 0));
    API_COMPILE(IntoRobot.defineDatapointNumber(1,DP_PERMISSION_UP_ONLY, -100.00, 100.00, 0.01));
    API_COMPILE(IntoRobot.defineDatapointNumber(1,DP_PERMISSION_UP_ONLY, -100.00, 100.00, 0.01, 0));
    API_COMPILE(IntoRobot.defineDatapointNumber(1,DP_PERMISSION_UP_ONLY, -100.00, 100.00, 0.01, 0, DP_POLICY_TIMED));
    API_COMPILE(IntoRobot.defineDatapointNumber(1,DP_PERMISSION_UP_ONLY, -100.00, 100.00, 0.01, 0, DP_POLICY_TIMED, 0));

    API_COMPILE(IntoRobot.defineDatapointEnum(1,DP_PERMISSION_UP_ONLY));
    API_COMPILE(IntoRobot.defineDatapointEnum(1,DP_PERMISSION_UP_ONLY, 0));
    API_COMPILE(IntoRobot.defineDatapointEnum(1,DP_PERMISSION_UP_ONLY, 0, DP_POLICY_TIMED));
    API_COMPILE(IntoRobot.defineDatapointEnum(1,DP_PERMISSION_UP_ONLY, 0, DP_POLICY_TIMED, 0));

    API_COMPILE(IntoRobot.defineDatapointString(1,DP_PERMISSION_UP_ONLY));
    API_COMPILE(IntoRobot.defineDatapointString(1,DP_PERMISSION_UP_ONLY, "oh yeah!"));
    API_COMPILE(IntoRobot.defineDatapointString(1,DP_PERMISSION_UP_ONLY, "oh yeah!", DP_POLICY_TIMED));
    API_COMPILE(IntoRobot.defineDatapointString(1,DP_PERMISSION_UP_ONLY, "oh yeah!", DP_POLICY_TIMED, 0));

    API_COMPILE(IntoRobot.defineDatapointBinary(1,DP_PERMISSION_UP_ONLY));
    API_COMPILE(IntoRobot.defineDatapointBinary(1,DP_PERMISSION_UP_ONLY, "\x31\x32\x33\x34\x35", 5));
    API_COMPILE(IntoRobot.defineDatapointBinary(1,DP_PERMISSION_UP_ONLY, "\x31\x32\x33\x34\x35", 5, DP_POLICY_TIMED));
    API_COMPILE(IntoRobot.defineDatapointBinary(1,DP_PERMISSION_UP_ONLY, "\x31\x32\x33\x34\x35", 5, DP_POLICY_TIMED, 0));
}

test(api_intorobot_readDatapoint) {
    bool valueBool = 0;
    int  valueInt = 0;
    int32_t valueInt32 = 0;
    uint32_t valueUint32 = 0;
    float valueFloat = 0;
    double valueDouble = 0;
    char valueString[50];
    String valueSmartString;
    uint8_t valueBinary[50];
    uint16_t len;

    API_COMPILE(IntoRobot.readDatapoint(1, valueBool));
    API_COMPILE(IntoRobot.readDatapoint(1, valueInt));
    API_COMPILE(IntoRobot.readDatapoint(1, valueInt32));
    API_COMPILE(IntoRobot.readDatapoint(1, valueUint32));
    API_COMPILE(IntoRobot.readDatapoint(1, valueFloat));
    API_COMPILE(IntoRobot.readDatapoint(1, valueDouble));
    API_COMPILE(IntoRobot.readDatapoint(1, valueSmartString));
    API_COMPILE(IntoRobot.readDatapoint(1, valueString));
    API_COMPILE(IntoRobot.readDatapoint(1, valueBinary, len));
}

test(api_intorobot_writeDatapoint) {
    bool valueBool = 0;
    int valueInt = 0;
    int8_t valueInt8 = 0;
    uint8_t valueUint8 = 0;
    int16_t valueInt16 = 0;
    uint16_t valueUint16 = 0;
    int32_t valueInt32 = 0;
    uint32_t valueUint32 = 0;
    float valueFloat = 0;
    double valueDouble = 0;
    char valueString[] = "oh yeah!";
    String valueSmartString(valueString);


    API_COMPILE(IntoRobot.writeDatapoint(1, true));
    API_COMPILE(IntoRobot.writeDatapoint(1, 323));
    API_COMPILE(IntoRobot.writeDatapoint(1, 323.32));
    API_COMPILE(IntoRobot.writeDatapoint(1, "oh yeah!"));
    API_COMPILE(IntoRobot.writeDatapoint(1, "\x31\x32\x33\x34\x35", 5));

    API_COMPILE(IntoRobot.writeDatapoint(1, valueBool));
    API_COMPILE(IntoRobot.writeDatapoint(1, valueInt));
    API_COMPILE(IntoRobot.writeDatapoint(1, valueInt8));
    API_COMPILE(IntoRobot.writeDatapoint(1, valueUint8));
    API_COMPILE(IntoRobot.writeDatapoint(1, valueInt16));
    API_COMPILE(IntoRobot.writeDatapoint(1, valueUint16));
    API_COMPILE(IntoRobot.writeDatapoint(1, valueInt32));
    API_COMPILE(IntoRobot.writeDatapoint(1, valueUint32));
    API_COMPILE(IntoRobot.writeDatapoint(1, valueFloat));
    API_COMPILE(IntoRobot.writeDatapoint(1, valueDouble));
    API_COMPILE(IntoRobot.writeDatapoint(1, valueString));
    API_COMPILE(IntoRobot.writeDatapoint(1, valueSmartString));
    API_COMPILE(IntoRobot.writeDatapoint(1, valueString, strlen(valueString)));
    API_COMPILE(IntoRobot.writeDatapoints());
}

test(api_spark_syncTime) {
    API_COMPILE(IntoRobot.syncTime());
}

test(api_intorobot_connection) {
    bool connected = false;
    API_COMPILE(connected=IntoRobot.connected());
    connected++;
    API_COMPILE(IntoRobot.connect());
    API_COMPILE(IntoRobot.disconnect());
    API_COMPILE(IntoRobot.process());
}

test(api_spark_deviceID) {
    String id;
    API_COMPILE(id = IntoRobot.deviceID());
}

test(api_spark_debug) {
    API_COMPILE(IntoRobot.available());
    API_COMPILE(IntoRobot.read());
}

#endif
