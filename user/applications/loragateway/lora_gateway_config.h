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

#ifndef LORA_GATEWAY_CONFIG_H_
#define LORA_GATEWAY_CONFIG_H_

#include "arduino.h"
// ----------------------------------------------------------------------------------------

// The spreading factor is the most important parameter to set for a single channel
// gateway. It specifies the speed/datarate in which the gateway and node communicate.
// As the name says, in principle the single channel gateway listens to one channel/frequency
// and to one spreading factor only.
// This parameters contains the default value of SF, the actual version can be set with
// the webserver and it will be stored in SPIFF
#define _SPREADING         SF7     // Send and receive on this Spreading Factor (only)

// Single channel gateways if they behave strict should only use one frequency
// channel and one spreading factor. However, the TTN backend replies on RX2
// timeslot for spreading factors SF9-SF12.
// Also, the server will respond with SF12 in the RX2 timeslot.
// If the 1ch gateway is working in and for nodes that ONLY transmit and receive on the set
// and agreed frequency and spreading factor. make sure to set STRICT to 1.
// In this case, the frequency and spreading factor for downlink messages is adapted by this
// gateway
// NOTE: If your node has only one frequency enabled and one SF, you must set this to 1
//      in order to receive downlink messages
#define _STRICT_1CH        0     // 1 is strict, 0 is as driven by backend

// Channel Activity Detection
// This function will scan for valid LoRa headers and determine the Spreading
// factor accordingly. If set to 1 we will use this function which means the
// 1-channel gateway will become even more versatile. If set to 0 we will use the
// continuous listen mode.
// Using this function means that we HAVE to use more dio pins on the RFM95/sx1276
// device and also connect enable dio1 to detect this state.
#define _CAD               0

// Gather statistics on sensor and Wifi status
#define STATISTICS         2
// Maximum number of statistics records gathered. 20 is a good maximum
#define MAX_STAT           20

// Initial value of debug parameter. Can be hanged using the admin webserver
// For operational use, set initial DEBUG vaulue 0
#define DEBUG              1

// Define the correct radio type that you are using
#define CFG_sx1276_radio
//#define CFG_sx1272_radio

// Set the Server Settings (IMPORTANT)
#define _LOCUDPPORT        1700                     // Often 1700 or 1701 is used for upstream comms

#define _PULL_INTERVAL     30                       // PULL_DATA messages to server to get downstream
#define _STAT_INTERVAL     65                       // Send a 'stat' message to server

// loragateway udp definitions
#define _INTOROBOT_LORA_PORT           1700
#define _INTOROBOT_LORA_SERVER         "115.29.177.202"

// Gateway Ident definitions
#define _DESCRIPTION       "IntoRobot LoraGateway"
#define _EMAIL             "robin@molmc.com"
#define _PLATFORM          "IntoRobot"
#define _LAT               52.0000
#define _LON               5.90000
#define _ALT               1

// Definitions for the admin webserver
#define A_SERVER           1              // Define local WebServer only if this define is set
#define SERVERPORT         80           // local webserver port

#define A_MAXBUFSIZE       192        // Must be larger than 128, but small enough to work
#define _BAUDRATE          115200        // Works for debug messages to serial momitor

void gway_failed(const char *file, uint16_t line);

#if !defined(CFG_noassert)
#define ASSERT(cond) if(!(cond)) gway_failed(__FILE__, __LINE__)
#else
#define ASSERT(cond) /**/
#endif

#endif
