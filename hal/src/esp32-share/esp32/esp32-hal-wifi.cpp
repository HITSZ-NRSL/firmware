/*
   ESP8266WiFiGeneric.cpp - WiFi library for esp8266

   Copyright (c) 2014 Ivan Grokhotkov. All rights reserved.
   This file is part of the esp8266 core for Arduino environment.

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

   Reworked on 28 Dec 2015 by Markus Sattler

*/


#include <string.h>
#include "timer_hal.h"
#include "service_debug.h"
#include "delay_hal.h"

extern "C" {
#include "lwip/dns.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_smartconfig.h"
}

#include "esp32-hal-wifi.h"

static ScanDoneCb _scanDoneCb = NULL;
static wl_status_t _wifiStatus;

static volatile uint32_t esp32_wifi_timeout_start;
static volatile uint32_t esp32_wifi_timeout_duration;

inline void ARM_WIFI_TIMEOUT(uint32_t dur) {
    esp32_wifi_timeout_start = HAL_Timer_Get_Milli_Seconds();
    esp32_wifi_timeout_duration = dur;
    //DEBUG("esp32 WIFI WD Set %d",(dur));
}
inline bool IS_WIFI_TIMEOUT() {
    return esp32_wifi_timeout_duration && ((HAL_Timer_Get_Milli_Seconds()-esp32_wifi_timeout_start)>esp32_wifi_timeout_duration);
}

inline void CLR_WIFI_TIMEOUT() {
    esp32_wifi_timeout_duration = 0;
    //DEBUG("esp32 WIFI WD Cleared, was %d", esp32_wifi_timeout_duration);
}

static void _setStatus(wl_status_t status)
{
    _wifiStatus = status;
}

static esp_err_t _eventCallback(void *arg, system_event_t *event)
{
    if(event->event_id == SYSTEM_EVENT_SCAN_DONE) {
        _scanDoneCb();
    }
    else if(event->event_id == SYSTEM_EVENT_STA_DISCONNECTED) {
        uint8_t reason = event->event_info.disconnected.reason;
        if(reason == WIFI_REASON_NO_AP_FOUND) {
            _setStatus(WL_NO_SSID_AVAIL);
        }
        else if(reason == WIFI_REASON_AUTH_FAIL || reason == WIFI_REASON_ASSOC_FAIL) {
            _setStatus(WL_CONNECT_FAILED);
        }
        else if(reason == WIFI_REASON_BEACON_TIMEOUT || reason == WIFI_REASON_HANDSHAKE_TIMEOUT) {
            _setStatus(WL_CONNECTION_LOST);
        }
        else {
            _setStatus(WL_DISCONNECTED);
        }
    }
    else if(event->event_id == SYSTEM_EVENT_STA_START) {
        _setStatus(WL_DISCONNECTED);
    }
    else if(event->event_id == SYSTEM_EVENT_STA_STOP) {
        _setStatus(WL_NO_SHIELD);
    }
    else if(event->event_id == SYSTEM_EVENT_STA_GOT_IP) {
        //DEBUG("SYSTEM_EVENT_STA_GOT_IP");
        tcpip_adapter_ip_info_t ip;
        tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip);
        //DEBUG("%x", ip.ip.addr);

        _setStatus(WL_CONNECTED);
    }
    return ESP_OK;
}

static bool wifiLowLevelInit(){
    static bool lowLevelInitDone = false;
    if(!lowLevelInitDone){
        tcpip_adapter_init();
        esp_event_loop_init(&_eventCallback, NULL);
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        esp_err_t err = esp_wifi_init(&cfg);
        if(err){
            return false;
        }
        esp_wifi_set_storage(WIFI_STORAGE_FLASH);
        esp_wifi_set_mode(WIFI_MODE_NULL);
        lowLevelInitDone = true;
    }
    return true;
}

static bool wifiLowLevelDeinit(){
    //deinit not working yet!
    esp_wifi_deinit();
    return true;
}

static bool _esp_wifi_started = false;

static bool espWiFiStart() {
    if(_esp_wifi_started) {
        return true;
    }
    if(!wifiLowLevelInit()) {
        return false;
    }
    esp_err_t err = esp_wifi_start();
    if (err != ESP_OK) {
        wifiLowLevelDeinit();
        return false;
    }
    _esp_wifi_started = true;
    return true;
}

static bool espWiFiStop() {
    esp_err_t err;
    if(!_esp_wifi_started) {
        return true;
    }
    err = esp_wifi_stop();
    if(err) {
        return false;
    }
    _esp_wifi_started = false;
    return wifiLowLevelDeinit();
}

void esp32_setScanDoneCb(ScanDoneCb cb)
{
    if (cb != NULL)
    {
        _scanDoneCb = cb;
    }
}

bool esp32_setMode(wifi_mode_t m)
{
    wifi_mode_t cm = esp32_getMode();
    if(cm == WIFI_MODE_MAX) {
        return false;
    }
    if(cm == m) {
        return true;
    }
    esp_err_t err;
    err = esp_wifi_set_mode(m);
    if(err) {
        return false;
    }
    if(m) {
        return espWiFiStart();
    }
    return espWiFiStop();
}

wifi_mode_t esp32_getMode()
{
    if(!wifiLowLevelInit()){
        return WIFI_MODE_MAX;
    }

    uint8_t mode;
    esp_wifi_get_mode((wifi_mode_t*)&mode);
    return (wifi_mode_t)mode;
}

bool esp32_enableSTA(bool enable)
{
    wifi_mode_t currentMode = esp32_getMode();
    bool isEnabled = ((currentMode & WIFI_MODE_STA) != 0);

    if(isEnabled != enable) {
        if(enable) {
            return esp32_setMode((wifi_mode_t)(currentMode | WIFI_MODE_STA));
        } else {
            return esp32_setMode((wifi_mode_t)(currentMode & (~WIFI_MODE_STA)));
        }
    } else {
        return true;
    }
}

bool esp32_enableAP(bool enable)
{
    wifi_mode_t currentMode = esp32_getMode();
    bool isEnabled = ((currentMode & WIFI_MODE_AP) != 0);

    if(isEnabled != enable) {
        if(enable) {
            return esp32_setMode((wifi_mode_t)(currentMode | WIFI_MODE_AP));
        } else {
            return esp32_setMode((wifi_mode_t)(currentMode & (~WIFI_MODE_AP)));
        }
    } else {
        return true;
    }
}

uint8_t* esp32_getMacAddress(uint8_t* mac)
{
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    return mac;
}

bool esp32_setDHCP(char enable)
{
    if(true == enable) {
        tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_STA);
    } else {
        tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA);
    }
    return true;
}

bool esp32_setAutoConnect(bool autoConnect)
{
    bool ret;
    ret = esp_wifi_set_auto_connect(autoConnect);
    return ret;
}

bool esp32_getAutoConnect()
{
    bool autoConnect;
    esp_wifi_get_auto_connect(&autoConnect);
    return autoConnect;
}

bool esp32_setAutoReconnect(bool autoReconnect)
{
    if((esp32_getMode() & WIFI_MODE_STA) != 0) {
        if(esp_wifi_disconnect() == ESP_OK) {
            return esp_wifi_connect() == ESP_OK;
        }
    }
    return false;
}

int32_t esp32_getRSSI(void)
{
    wifi_ap_record_t info;
    if(!esp_wifi_sta_get_ap_info(&info)) {
        return info.rssi;
    }
    return 0;
}

int esp32_setConfig(wifi_config_t *conf)
{
    esp_wifi_set_config(WIFI_IF_STA, conf);
    return 0;
}

static bool _smartConfigStarted = false;
static bool _smartConfigDone = false;

static void smartConfigCallback(uint32_t st, void* result)
{
    smartconfig_status_t status = (smartconfig_status_t) st;
    DEBUG("status = %d", status);
    if (status == SC_STATUS_LINK) {
        wifi_sta_config_t *sta_conf = reinterpret_cast<wifi_sta_config_t *>(result);
        DEBUG("ssid     = %s", sta_conf->ssid);
        DEBUG("password = %s", sta_conf->password);
        esp_wifi_set_config(WIFI_IF_STA, (wifi_config_t *)sta_conf);
        esp_wifi_connect();
    } else if (status == SC_STATUS_LINK_OVER) {
        _smartConfigDone = true;
    }
}

bool esp32_beginSmartConfig()
{
    if (_smartConfigStarted) {
        return false;
    }

    if (!esp32_setMode(WIFI_MODE_STA)) {
        return false;
    }

    esp_wifi_disconnect();

    esp_err_t err;
    err = esp_smartconfig_start(reinterpret_cast<sc_callback_t>(&smartConfigCallback), 1);
    if (err == ESP_OK) {
        _smartConfigStarted = true;
        _smartConfigDone = false;
        return true;
    }
    return false;
}

bool esp32_stopSmartConfig()
{
    if (!_smartConfigStarted) {
        return true;
    }

    if (esp_smartconfig_stop() == ESP_OK) {
        _smartConfigStarted = false;
        return true;
    }

    return false;
}

static bool esp32_smartConfigDone()
{
    if(!_smartConfigStarted) {
        return false;
    }
    return _smartConfigDone;
}

static bool _dns_busy = false;

static void wifi_dns_found_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg)
{
    if(ipaddr) {
        (*reinterpret_cast<uint32_t*>(callback_arg)) = ipaddr->u_addr.ip4.addr;
    }
    _dns_busy = false;
}

int esp32_gethostbyname(const char* hostname, uint16_t hostnameLen, uint32_t &ip_addr)
{
    ip_addr_t addr;

    if(!strcmp(hostname, "255.255.255.255")) {
        ip_addr = IPADDR_NONE;
        return 0;
    }

    addr.u_addr.ip4.addr = ipaddr_addr(hostname);
    if (addr.u_addr.ip4.addr != IPADDR_NONE) {
        ip_addr = addr.u_addr.ip4.addr;
        return 0;
    }

    _dns_busy = true;
    err_t err = dns_gethostbyname(hostname, &addr, &wifi_dns_found_callback, &ip_addr);
    if(err == ERR_OK && addr.u_addr.ip4.addr) {
        ip_addr = addr.u_addr.ip4.addr;
        _dns_busy = false;
    } else if(err == ERR_INPROGRESS) {
        ARM_WIFI_TIMEOUT(2000);
        while (_dns_busy) {
            HAL_Delay_Milliseconds(100);
            if(IS_WIFI_TIMEOUT()) {
                CLR_WIFI_TIMEOUT();
                break;
            }
        }
    } else {
        _dns_busy = false;
        return 1;
    }
    return 0;
}

int esp32_connect()
{
    if ( esp_wifi_connect() == ESP_OK ) {
        return 0;
    }
    return 1;
}

int esp32_disconnect()
{
    if ( esp_wifi_disconnect() == ESP_OK ) {
        return 0;
    }
    return 1;
}

wl_status_t esp32_status()
{
    return _wifiStatus;
}

