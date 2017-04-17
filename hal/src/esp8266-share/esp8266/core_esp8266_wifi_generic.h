/*
 ESP8266WiFiGeneric.h - esp8266 Wifi support.
 Based on WiFi.h from Ardiono WiFi shield library.
 Copyright (c) 2011-2014 Arduino.  All right reserved.
 Modified by Ivan Grokhotkov, December 2014
 Reworked by Markus Sattler, December 2015

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef ESP8266_WIFI_GENERIC_H_
#define ESP8266_WIFI_GENERIC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "esp8266_wifi_type.h"
#include "wl_definitions.h"

bool esp8266_wifiInit(void);
bool esp8266_setMode(WiFiMode_t m);
WiFiMode_t esp8266_getMode();
bool esp8266_enableSTA(bool enable);
bool esp8266_enableAP(bool enable);
uint8_t* esp8266_getMacAddress(uint8_t* mac);
bool esp8266_setDHCP(char enable);
bool esp8266_setAutoConnect(bool autoConnect);
int32_t esp8266_getRSSI(void);
bool esp8266_beginSmartConfig();
bool esp8266_stopSmartConfig();
bool esp8266_smartConfigDone();
int esp8266_gethostbyname(const char* hostname, uint16_t hostnameLen, uint32_t &ip_addr);
int esp8266_connect();
int esp8266_disconnect();
wl_status_t esp8266_status();
bool esp8266_setAutoConnect(bool autoConnect);
bool esp8266_getAutoConnect();
bool esp8266_setAutoReconnect(bool autoReconnect);

#ifdef __cplusplus
}
#endif


#endif /* ESP8266_WIFI_GENERIC_H_ */
