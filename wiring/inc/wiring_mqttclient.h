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

#ifndef WIRING_MQTTCLIENT_H_
#define WIRING_MQTTCLIENT_H_

#include "intorobot_config.h"

#ifndef configNO_CLOUD
#include "wiring_constants.h"
#include "wiring_ipaddress.h"
#include "wiring_tcpclient.h"

#define MQTT_VERSION_3_1      3
#define MQTT_VERSION_3_1_1    4

// MQTT_VERSION : Pick the version
//#define MQTT_VERSION MQTT_VERSION_3_1
#ifndef MQTT_VERSION
#define MQTT_VERSION MQTT_VERSION_3_1_1
#endif

// MQTT_MAX_PACKET_SIZE : Maximum packet size
#ifndef MQTT_MAX_PACKET_SIZE
#define MQTT_MAX_PACKET_SIZE 512+256
#endif

// MQTT_KEEPALIVE : keepAlive interval in Seconds
#ifndef MQTT_KEEPALIVE
#define MQTT_KEEPALIVE 60
#endif

// MQTT_SOCKET_TIMEOUT: socket timeout interval in Seconds
#ifndef MQTT_SOCKET_TIMEOUT
#define MQTT_SOCKET_TIMEOUT 15
#endif

// MQTT_MAX_TRANSFER_SIZE : limit how much data is passed to the network client
//  in each write call. Needed for the Arduino Wifi Shield. Leave undefined to
//  pass the entire MQTT packet in each write call.
//#define MQTT_MAX_TRANSFER_SIZE 80

// Possible values for client.state()
#define MQTT_CONNECTION_TIMEOUT     -4
#define MQTT_CONNECTION_LOST        -3
#define MQTT_CONNECT_FAILED         -2
#define MQTT_DISCONNECTED           -1
#define MQTT_CONNECTED               0
#define MQTT_CONNECT_BAD_PROTOCOL    1
#define MQTT_CONNECT_BAD_CLIENT_ID   2
#define MQTT_CONNECT_UNAVAILABLE     3
#define MQTT_CONNECT_BAD_CREDENTIALS 4
#define MQTT_CONNECT_UNAUTHORIZED    5

#define MQTTCONNECT     1 << 4  // Client request to connect to Server
#define MQTTCONNACK     2 << 4  // Connect Acknowledgment
#define MQTTPUBLISH     3 << 4  // Publish message
#define MQTTPUBACK      4 << 4  // Publish Acknowledgment
#define MQTTPUBREC      5 << 4  // Publish Received (assured delivery part 1)
#define MQTTPUBREL      6 << 4  // Publish Release (assured delivery part 2)
#define MQTTPUBCOMP     7 << 4  // Publish Complete (assured delivery part 3)
#define MQTTSUBSCRIBE   8 << 4  // Client Subscribe request
#define MQTTSUBACK      9 << 4  // Subscribe Acknowledgment
#define MQTTUNSUBSCRIBE 10 << 4 // Client Unsubscribe request
#define MQTTUNSUBACK    11 << 4 // Unsubscribe Acknowledgment
#define MQTTPINGREQ     12 << 4 // PING Request
#define MQTTPINGRESP    13 << 4 // PING Response
#define MQTTDISCONNECT  14 << 4 // Client is Disconnecting
#define MQTTReserved    15 << 4 // Reserved

#define MQTTQOS0        (0 << 1)
#define MQTTQOS1        (1 << 1)
#define MQTTQOS2        (2 << 1)

#define MQTT_CALLBACK_SIGNATURE void (*callback)(char*, uint8_t*, uint32_t)

class MqttClientClass {
private:
    Client* _client;
    uint8_t buffer[MQTT_MAX_PACKET_SIZE];
    uint16_t nextMsgId;
    unsigned long lastOutActivity;
    unsigned long lastInActivity;
    bool pingOutstanding;
    MQTT_CALLBACK_SIGNATURE;
    uint16_t readPacket(uint8_t*);
    boolean readByte(uint8_t * result);
    boolean readByte(uint8_t * result, uint16_t * index);
    boolean write(uint8_t header, uint8_t* buf, uint16_t length);
    uint16_t writeString(const char* string, uint8_t* buf, uint16_t pos);
    IPAddress ip;
    const char* domain;
    uint16_t port;
    Stream* stream;
    int _state;
    uint16_t keepAlive;

public:
    MqttClientClass();
    MqttClientClass(Client& client);
    MqttClientClass(IPAddress, uint16_t, Client& client);
    MqttClientClass(IPAddress, uint16_t, Client& client, Stream&);
    MqttClientClass(IPAddress, uint16_t, MQTT_CALLBACK_SIGNATURE,Client& client);
    MqttClientClass(IPAddress, uint16_t, MQTT_CALLBACK_SIGNATURE,Client& client, Stream&);
    MqttClientClass(uint8_t *, uint16_t, Client& client);
    MqttClientClass(uint8_t *, uint16_t, Client& client, Stream&);
    MqttClientClass(uint8_t *, uint16_t, MQTT_CALLBACK_SIGNATURE,Client& client);
    MqttClientClass(uint8_t *, uint16_t, MQTT_CALLBACK_SIGNATURE,Client& client, Stream&);
    MqttClientClass(const char*, uint16_t, Client& client);
    MqttClientClass(const char*, uint16_t, Client& client, Stream&);
    MqttClientClass(const char*, uint16_t, MQTT_CALLBACK_SIGNATURE,Client& client);
    MqttClientClass(const char*, uint16_t, MQTT_CALLBACK_SIGNATURE,Client& client, Stream&);

    MqttClientClass& setServer(IPAddress ip, uint16_t port);
    MqttClientClass& setServer(uint8_t * ip, uint16_t port);
    MqttClientClass& setServer(const char * domain, uint16_t port);
    MqttClientClass& setCallback(MQTT_CALLBACK_SIGNATURE);
    MqttClientClass& setClient(Client& client);
    MqttClientClass& setStream(Stream& stream);
    MqttClientClass& setKeepAlive(uint16_t sec);

    boolean connect(const char* id);
    boolean connect(const char* id, const char* user, const char* pass);
    boolean connect(const char* id, const char* willTopic, uint8_t willQos, boolean willRetain, const char* willMessage);
    boolean connect(const char* id, const char* user, const char* pass, const char* willTopic, uint8_t willQos, boolean willRetain, const char* willMessage);
    void disconnect();
    boolean publish(const char* topic, const char* payload);
    boolean publish(const char* topic, const char* payload, boolean retained);
    boolean publish(const char* topic, const uint8_t * payload, unsigned int plength);
    boolean publish(const char* topic, const uint8_t * payload, unsigned int plength, boolean retained);
    boolean publish_P(const char* topic, const uint8_t * payload, unsigned int plength, boolean retained);
    boolean subscribe(const char* topic);
    boolean subscribe(const char* topic, uint8_t qos);
    boolean unsubscribe(const char* topic);
    boolean loop();
    boolean connected();
    int state();
};

#endif

#endif
