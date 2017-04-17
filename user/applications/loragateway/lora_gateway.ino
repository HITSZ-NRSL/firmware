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

#include "lora_gateway_config.h"        // This file contains configuration of GWay
#include "lora_gateway.h"        // This file contains configuration of GWay
#include "lora_sx1276.h"

//SerialDebugOutput debugOutput(115200, ALL_LEVEL);

SYSTEM_MODE(MANUAL)
PRODUCT_ID(OhFps3PRwNGodYNaeqUx0m67OPn2UV51)
PRODUCT_SECRET(70d344cfcc82e8566c9d7546dc4e3e99)
PRODUCT_VERSION(1)

unsigned long nowTime=0;
struct LoraGwayConfig gwayConfig;

#define VERSION " ! V. 4.0.2, 170129"

using namespace std;

byte currentMode = 0x81;

uint32_t cp_nb_rx_rcv;
uint32_t cp_nb_rx_ok;
uint32_t cp_nb_rx_bad;
uint32_t cp_nb_rx_nocrc;
uint32_t cp_up_pkt_fwd;

uint8_t MAC_array[6];
char MAC_char[19];

/*******************************************************************************
 *
 * Configure these values only if necessary!
 *
 *******************************************************************************/
// Set location, description and other configuration parameters
// Defined in lora_gateway.h
float lat = _LAT;                   // Configuration specific info...
float lon = _LON;
int   alt = _ALT;
char platform[24] = _PLATFORM;      // platform definition
char email[40] = _EMAIL;            // used for contact email
char description[64]= _DESCRIPTION; // used for free form description

// define servers
IPAddress ttnServer;                // IP Address of thethingsnetwork server

UDP Udp;
uint32_t stattime = 0;              // last time we sent a stat message to server
uint32_t pulltime = 0;              // last time we sent a pull_data request to server
uint32_t lastTmst = 0;

#define TX_BUFF_SIZE  2048              // Upstream buffer to send to MQTT
#define RX_BUFF_SIZE  1024              // Downstream received from MQTT
#define STATUS_SIZE   512               // Should(!) be enough based on the static text .. was 1024

uint8_t buff_up[TX_BUFF_SIZE];      // buffer to compose the upstream packet to backend server
uint8_t buff_down[RX_BUFF_SIZE];    // Buffer for downstream
uint16_t lastToken = 0x00;

#if GATEWAYNODE==1
uint16_t frameCount=0;              // We write this to SPIFF file
#endif

// ----------------------------------------------------------------------------
// DIE is not use actively in the source code anymore.
// It is replaced by a Serial.print command so we know that we have a problem
// somewhere.
// There are at least 3 other ways to restart the ESP. Pick one if you want.
// ----------------------------------------------------------------------------
void die(const char *s)
{
    Serial.println(s);
    delay(50);
    // system_restart();                        // SDK function
    // ESP.reset();
    abort();                                    // Within a second
}

// ----------------------------------------------------------------------------
// gway_failed is a function called by ASSERT.
// ----------------------------------------------------------------------------
void gway_failed(const char *file, uint16_t line) {
    Serial.print(F("Program failed in file: "));
    Serial.print(file);
    Serial.print(F(", line: "));
    Serial.print(line);
}

// ----------------------------------------------------------------------------
// Print leading '0' digits for hours(0) and second(0) when
// printing values less than 10
// ----------------------------------------------------------------------------
void printDigits(int digits)
{
    // utility function for digital clock display: prints preceding colon and leading 0
    if(digits < 10)
        Serial.print(F("0"));
    Serial.print(digits);
}

// ----------------------------------------------------------------------------
// Print utin8_t values in HEX with leading 0 when necessary
// ----------------------------------------------------------------------------
void printHexDigit(uint8_t digit)
{
    // utility function for printing Hex Values with leading 0
    if(digit < 0x10)
        Serial.print('0');
    Serial.print(digit,HEX);
}

// ----------------------------------------------------------------------------
// Print the current time
// ----------------------------------------------------------------------------
void printTime() {
    //char *Days [] = {"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
    String Days [] = {"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
    Serial.print(Days[Time.weekday()-1]);
    Serial.print(F(" "));
    printDigits(Time.hour());
    Serial.print(F(":"));
    printDigits(Time.minute());
    Serial.print(F(":"));
    printDigits(Time.second());
    return;
}

// ----------------------------------------------------------------------------
// Convert a float to string for printing
// f is value to convert
// p is precision in decimal digits
// val is character array for results
// ----------------------------------------------------------------------------
void ftoa(float f, char *val, int p) {
    int j=1;
    int ival, fval;
    char b[6];

    for (int i=0; i< p; i++) { j= j*10; }

    ival = (int) f;        // Make integer part
    fval = (int) ((f- ival)*j);     // Make fraction. Has same sign as integer part
    if (fval<0) fval = -fval;     // So if it is negative make fraction positive again.
    // sprintf does NOT fit in memory
    strcat(val,itoa(ival,b,10));
    strcat(val,".");       // decimal point

    itoa(fval,b,10);
    for (int i=0; i<(p-strlen(b)); i++) strcat(val,"0");
    // Fraction can be anything from 0 to 10^p , so can have less digits
    strcat(val,b);
}

// ----------------------------------------------------------------------------
// Read DOWN a package from UDP socket, can come from any server
// Messages are received when server responds to gateway requests from LoRa nodes
// (e.g. JOIN requests etc.) or when server has downstream data.
// We repond only to the server that sent us a message!
// ----------------------------------------------------------------------------
int readUdp(int packetSize, uint8_t * buff_down)
{
    uint8_t protocol;
    uint16_t token;
    uint8_t ident;
    char LoraBuffer[64];       //buffer to hold packet to send to LoRa node

    if (packetSize > RX_BUFF_SIZE) {
        Serial.print(F("readUDP:: ERROR package of size: "));
        Serial.println(packetSize);
        Udp.flush();
        return(-1);
    }

    Udp.read(buff_down, packetSize);
    IPAddress remoteIpNo = Udp.remoteIP();
    unsigned int remotePortNo = Udp.remotePort();

    uint8_t * data = buff_down + 4;
    protocol = buff_down[0];
    token = buff_down[2]*256 + buff_down[1];
    ident = buff_down[3];

    // now parse the message type from the server (if any)
    switch (ident) {
        case PKT_PUSH_DATA: // 0x00 UP
            if (debug >=1) {
                Serial.print(F("PKT_PUSH_DATA:: size ")); Serial.print(packetSize);
                Serial.print(F(" From ")); Serial.print(remoteIpNo);
                Serial.print(F(", port ")); Serial.print(remotePortNo);
                Serial.print(F(", data: "));
                for (int i=0; i<packetSize; i++) {
                    Serial.print(buff_down[i],HEX);
                    Serial.print(':');
                }
                Serial.println();
            }
            break;
        case PKT_PUSH_ACK: // 0x01 DOWN
            if (debug >= 2) {
                Serial.print(F("PKT_PUSH_ACK:: size ")); Serial.print(packetSize);
                Serial.print(F(" From ")); Serial.print(remoteIpNo);
                Serial.print(F(", port ")); Serial.print(remotePortNo);
                Serial.print(F(", token: "));
                Serial.println(token, HEX);
                Serial.println();
            }
            break;
        case PKT_PULL_DATA: // 0x02 UP
            Serial.print(F(" Pull Data"));
            Serial.println();
            break;
        case PKT_PULL_RESP: // 0x03 DOWN
            lastTmst = micros();     // Store the tmst this package was received
            // Send to the LoRa Node first (timing) and then do messaging
            if (sendPacket(data, packetSize-4) < 0) {
                return(-1);
            }

            // Now respond with an PKT_PULL_ACK; 0x04 UP
            buff_up[0]=buff_down[0];
            buff_up[1]=buff_down[1];
            buff_up[2]=buff_down[2];
            buff_up[3]=PKT_PULL_ACK;
            buff_up[4]=0;

            // Only send the PKT_PULL_ACK to the UDP socket that just sent the data!!!
            Udp.beginPacket(remoteIpNo, remotePortNo);
            if (Udp.write((char *)buff_up, 4) != 4) {
                Serial.println("PKT_PULL_ACK:: Error writing Ack");
            }
            else {
                if (debug>=1) {
                    Serial.print(F("PKT_PULL_ACK:: tmst="));
                    Serial.println(micros());
                }
            }
            Udp.endPacket();

            if (debug >=1) {
                Serial.print(F("PKT_PULL_RESP:: size ")); Serial.print(packetSize);
                Serial.print(F(" From ")); Serial.print(remoteIpNo);
                Serial.print(F(", port ")); Serial.print(remotePortNo);
                Serial.print(F(", data: "));
                data = buff_down + 4;
                data[packetSize] = 0;
                Serial.print((char *)data);
                Serial.println(F("..."));
            }

            break;
        case PKT_PULL_ACK: // 0x04 DOWN; the server sends a PULL_ACK to confirm PULL_DATA receipt
            if (debug >= 2) {
                Serial.print(F("PKT_PULL_ACK:: size ")); Serial.print(packetSize);
                Serial.print(F(" From ")); Serial.print(remoteIpNo);
                Serial.print(F(", port ")); Serial.print(remotePortNo);
                Serial.print(F(", data: "));
                for (int i=0; i<packetSize; i++) {
                    Serial.print(buff_down[i],HEX);
                    Serial.print(':');
                }
                Serial.println();
            }
            break;
        default:
#if GATEWAYMGT==1
            // For simplicity, we send the first 4 bytes too
            gateway_mgt(packetSize, buff_down);
#else
#endif
            Serial.print(F(", ERROR ident not recognized: "));
            Serial.println(ident);
            break;
    }
    // For downstream messages, fill the buff_down buffer
    return packetSize;
}

// ----------------------------------------------------------------------------
// Send UP an UDP/DGRAM message to the MQTT server
// If we send to more than one host (not sure why) then we need to set sockaddr
// before sending.
// ----------------------------------------------------------------------------
void sendUdp(uint8_t * msg, int length) {
    int l;
    lastToken = msg[2]*256+msg[1];

#ifdef configWIRING_WIFI_ENABLE
    if (!WiFi.ready()) {
#else
    if (!Cellular.ready()) {
#endif

        Serial.println(F("sendUdp: ERROR not connected to WLAN"));
        Udp.flush();
#if 0
        if (WlanConnect() < 0) {
            Serial.print(F("sendUdp: ERROR connecting to WiFi"));
            return;
        }
#endif
        if (debug>=1) Serial.println(F("WiFi reconnected"));
        delay(10);
    }

    //send the update
    if ((l = Udp.write((char *)msg, length)) != length) {
        Serial.println("sendUdp:: Error write");
    }
    else {
        if (debug>=2) {
            Serial.print(F("sendUdp 1: sent "));
            Serial.print(l);
            Serial.println(F(" bytes"));
        }
    }
    Udp.endPacket();
    return;
}

// ----------------------------------------------------------------------------
// connect to UDP – returns true if successful or false if not
// ----------------------------------------------------------------------------
bool UDPconnect() {
    bool ret = false;
    unsigned int localPort = _LOCUDPPORT;   // To listen to return messages from WiFi
    if (debug>=1) {
        Serial.print(F("Local UDP port "));
        Serial.println(localPort);
    }

    if (Udp.begin(localPort) == 1) {
        Udp.beginPacket(_INTOROBOT_LORA_SERVER, (int)_INTOROBOT_LORA_PORT);
        if (debug>=1) Serial.println(F("Connection successful"));
        ret = true;
    }
    else{
        Serial.println("Connection failed");
    }
    return(ret);
}

// ----------------------------------------------------------------------------
// Send UP periodic Pull_DATA message to server to keepalive the connection
// and to invite the server to send downstream messages when these are available
// *2, par. 5.2
// - Protocol Version (1 byte)
// - Random Token (2 bytes)
// - PULL_DATA identifier (1 byte) = 0x02
// - Gateway unique identifier (8 bytes) = MAC address
// ----------------------------------------------------------------------------
void pullData() {

    uint8_t pullDataReq[12];       // status report as a JSON object
    int pullIndex=0;
    int i;

    // pre-fill the data buffer with fixed fields
    pullDataReq[0]  = PROTOCOL_VERSION;   // 0x01
    uint8_t token_h = (uint8_t)rand();    // random token
    uint8_t token_l = (uint8_t)rand();    // random token
    pullDataReq[1]  = token_h;
    pullDataReq[2]  = token_l;
    pullDataReq[3]  = PKT_PULL_DATA;      // 0x02

    // READ MAC ADDRESS OF ESP8266, and return unique Gateway ID consisting of MAC address and 2bytes 0xFF
    pullDataReq[4]  = MAC_array[0];
    pullDataReq[5]  = MAC_array[1];
    pullDataReq[6]  = MAC_array[2];
    pullDataReq[7]  = 0xFF;
    pullDataReq[8]  = 0xFF;
    pullDataReq[9]  = MAC_array[3];
    pullDataReq[10] = MAC_array[4];
    pullDataReq[11] = MAC_array[5];

    pullIndex = 12;                       // 12-byte header

    pullDataReq[pullIndex] = 0;           // add string terminator, for safety

    if (debug>= 2) {
        Serial.printf("PKT_PULL_DATA request: < %d > ", pullIndex);
        for (i=0; i<pullIndex; i++) {
            Serial.print(pullDataReq[i],HEX);    // DEBUG: display JSON stat
            Serial.print(':');
        }
        Serial.println();
    }
    //send the update
    sendUdp(pullDataReq, pullIndex);
}

// ----------------------------------------------------------------------------
// Send UP periodic status message to server even when we do not receive any
// data.
// Parameter is socketr to TX to
// ----------------------------------------------------------------------------
void sendstat() {

    uint8_t status_report[STATUS_SIZE];      // status report as a JSON object
    char stat_timestamp[32];                 // XXX was 24
    time_t t;
    char clat[10]={0};
    char clon[10]={0};

    int stat_index=0;

    // pre-fill the data buffer with fixed fields
    status_report[0]  = PROTOCOL_VERSION;   // 0x01
    status_report[3]  = PKT_PUSH_DATA;      // 0x00

    // READ MAC ADDRESS OF ESP8266, and return unique Gateway ID consisting of MAC address and 2bytes 0xFF
    status_report[4]  = MAC_array[0];
    status_report[5]  = MAC_array[1];
    status_report[6]  = MAC_array[2];
    status_report[7]  = 0xFF;
    status_report[8]  = 0xFF;
    status_report[9]  = MAC_array[3];
    status_report[10] = MAC_array[4];
    status_report[11] = MAC_array[5];

    uint8_t token_h   = (uint8_t)rand();    // random token
    uint8_t token_l   = (uint8_t)rand();    // random token
    status_report[1]  = token_h;
    status_report[2]  = token_l;
    stat_index = 12;                        // 12-byte header

    t = Time.now();                              // get timestamp for statistics

    // XXX Using CET as the current timezone. Change to your timezone
    sprintf(stat_timestamp, "%04d-%02d-%02d %02d:%02d:%02d CET", Time.year(),Time.month(),Time.day(),Time.hour(),Time.minute(),Time.second());

    ftoa(lat,clat,4);          // Convert lat to char array with 4 decimals
    ftoa(lon,clon,4);          // As Arduino CANNOT prints floats

    // Build the Status message in JSON format, XXX Split this one up...
    delay(1);

    int j = snprintf((char *)(status_report + stat_index), STATUS_SIZE-stat_index,
            "{\"stat\":{\"time\":\"%s\",\"lati\":%s,\"long\":%s,\"alti\":%i,\"rxnb\":%u,\"rxok\":%u,\"rxfw\":%u,\"ackr\":%u.0,\"dwnb\":%u,\"txnb\":%u,\"pfrm\":\"%s\",\"mail\":\"%s\",\"desc\":\"%s\"}}",
            stat_timestamp, clat, clon, (int)alt, cp_nb_rx_rcv, cp_nb_rx_ok, cp_up_pkt_fwd, 0, 0, 0,platform,email,description);

    if (debug >=1) { delay(1); }
    stat_index += j;
    status_report[stat_index] = 0;        // add string terminator, for safety

    if (debug>=2) {
        Serial.printf("\r\nstat update: < %d > %s", stat_index, (char *)(status_report+12)); // DEBUG: display JSON stat
    }

    //send the update
    // delay(1);
    sendUdp(status_report, stat_index);
    return;
}

// ----------------------------------------------------------------------------
// Setup code (one time)
// _state is S_INIT
// ----------------------------------------------------------------------------
void setup () {
    Serial.begin(_BAUDRATE);   // As fast as possible for bus

    // Pins are defined and set in loraModem.h
    pinMode(pins.ss, OUTPUT);
    pinMode(pins.rst, OUTPUT);
    pinMode(pins.rxtx, OUTPUT);
    pinMode(pins.dio0, INPUT);    // This pin is interrupt
    pinMode(pins.dio1, INPUT);    // This pin is interrupt

    // If we are here we are connected to WLAN
    // So now test the UDP function
    if (!UDPconnect()) {
        Serial.println(F("Error UDPconnect"));
    }
    delay(500);

    SPI.begin();
    SPI.setBitOrder(MSBFIRST);
#if 0
    SPI.setFrequency( SPIFREQ );   // <=10 MHz
#endif
    delay(500);
    // We choose the Gateway ID to be the Ethernet Address of our Gateway card
    // display results of getting hardware address
    Serial.printf("--------------------------------------\r\n");
    Serial.printf("Gateway ID: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\r\n", \
            MAC_array[0], MAC_array[1], MAC_array[2], 0xFF, 0xFF, MAC_array[3], MAC_array[4], MAC_array[5]);
    Serial.printf("Listening at SF %d on %f Mhz.\r\n", sf, (double)freq/1000000);
    Serial.printf("--------------------------------------\r\n");

    _state = S_INIT;
    initLoraModem();

    _state = S_RX;
    rxLoraModem();

    if (_cad) {
        Serial.printf("_cad\r\n");
        _state = S_SCAN;
        cadScanner();          // Always start at SF7
    }

    // init interrupt handlers for GPIO15 / D8,
    // we switch on HIGH interrupts
    if (pins.dio0 == pins.dio1) {
        attachInterrupt(pins.dio0, Interrupt, RISING);  // Share interrupts
    }
    else {
        attachInterrupt(pins.dio0, Interrupt_0, RISING); // Separate interrupts
        attachInterrupt(pins.dio1, Interrupt_1, RISING); // Separate interrupts
    }
    System.configEnd();
}

// ----------------------------------------------------------------------------
// LOOP
// This is the main program that is executed time and time again.
// We need to give way to the backend WiFi processing that
// takes place somewhere in the ESP8266 firmware and therefore
// we include yield() statements at important points.
//
// Note: If we spend too much time in user processing functions
// and the backend system cannot do its housekeeping, the watchdog
// function will be executed which means effectively that the
// program crashes.
//
// NOTE: For ESP make sure not to do lage array declarations in loop();
// ----------------------------------------------------------------------------
void loop ()
{
    uint32_t nowseconds;
    int packetSize;

    // Receive Lora messages waiting, if there are any. Most important function in loop()
    if (_state == S_RXDONE) {
        eventHandler();       // Is S_RXDONE read a message
    }

#if 0
    // The next section is emergency only. If posible we hop() in the state machine.
    // If hopping is enabled, and by lack of timer, we hop()
    // XXX Experimental, 2.5 ms between hops max
    nowTime = micros();
    if ((_hop) && (((long)(nowTime - hopTime)) > 2500)) {
        if ((_state == S_SCAN) && (sf==SF12)) {
            if (debug>=1) Serial.println(F("loop:: hop"));
            hop();
        }
        // XXX section below does not work wthout further work. It is the section with the MOST
        // influence on the HOP mode of operation (which is somewhat unexpected)
        // If we keep staying in another state, reset
        else if (((long)(nowTime - hopTime)) > 100000) {
            if (debug>=2) {
                Serial.print(F("loop:: reset, STATE=")); Serial.print(_state);
                Serial.print(F(", F=")); Serial.print(ifreq);
            }
            _state= S_SCAN;
            hop();
            if (debug>=2) {
                Serial.print(F("->")); Serial.println(ifreq);
            }
            // rxLoraModem();
            if (_cad) { _state= S_SCAN; cadScanner(); }
        }
        else if (debug>=3) { Serial.print(F(" state=")); Serial.println(_state); }
        inHop = false;       // Reset re-entrane protection of HOP
    }
#endif
#if 1
    // Receive UDP PUSH_ACK messages from server. (*2, par. 3.3)
    // This is important since the TTN broker will return confirmation
    // messages on UDP for every message sent by the gateway. So we have to consume them..
    // As we do not know when the server will respond, we test in every loop.
    //

    while( (packetSize = Udp.parsePacket()) > 0) {  // Length of UDP message waiting
        // Packet may be PKT_PUSH_ACK (0x01), PKT_PULL_ACK (0x03) or PKT_PULL_RESP (0x04)
        // This command is found in byte 4 (buff_down[3])
        if (readUdp(packetSize, buff_down) < 0) {
            Serial.println(F("readUDP error"));
        }
    }

    // stat PUSH_DATA message (*2, par. 4)
    nowseconds = (uint32_t) millis() /1000;
    if (nowseconds - stattime >= _STAT_INTERVAL) {  // Wake up every xx seconds
        sendstat();          // Show the status message and send to server
        stattime = nowseconds;
    }

    // send PULL_DATA message (*2, par. 4)
    nowseconds = (uint32_t) millis() /1000;
    if (nowseconds - pulltime >= _PULL_INTERVAL) {  // Wake up every xx seconds
        pullData();          // Send PULL_DATA message to server
        pulltime = nowseconds;
    }
#endif
}
