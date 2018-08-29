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
#include "tick_hal.h"
#include "delay_hal.h"
#include "molmc_log.h"

extern "C" {
#include "lwip/dns.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_smartconfig.h"
}

#include "esp32-hal-wifi.h"
#include "net_hal.h"

const static char *TAG = "hal-wifi";

static ScanDoneCb _scanDoneCb = NULL;

static volatile uint32_t esp32_wifi_timeout_start;
static volatile uint32_t esp32_wifi_timeout_duration;

inline void ARM_WIFI_TIMEOUT(uint32_t dur) {
    esp32_wifi_timeout_start = HAL_Tick_Get_Milli_Seconds();
    esp32_wifi_timeout_duration = dur;
    //MOLMC_LOGD(TAG, "esp32 WIFI WD Set %d",(dur));
}
inline bool IS_WIFI_TIMEOUT() {
    return esp32_wifi_timeout_duration && ((HAL_Tick_Get_Milli_Seconds()-esp32_wifi_timeout_start)>esp32_wifi_timeout_duration);
}

inline void CLR_WIFI_TIMEOUT() {
    esp32_wifi_timeout_duration = 0;
    //MOLMC_LOGD(TAG, "esp32 WIFI WD Cleared, was %d", esp32_wifi_timeout_duration);
}

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

static xQueueHandle _network_event_queue;
static TaskHandle_t _network_event_task_handle = NULL;

const char * system_event_reasons[] = { "UNSPECIFIED", "AUTH_EXPIRE", "AUTH_LEAVE", "ASSOC_EXPIRE", "ASSOC_TOOMANY", "NOT_AUTHED", "NOT_ASSOCED", "ASSOC_LEAVE", "ASSOC_NOT_AUTHED", "DISASSOC_PWRCAP_BAD", "DISASSOC_SUPCHAN_BAD", "IE_INVALID", "MIC_FAILURE", "4WAY_HANDSHAKE_TIMEOUT", "GROUP_KEY_UPDATE_TIMEOUT", "IE_IN_4WAY_DIFFERS", "GROUP_CIPHER_INVALID", "PAIRWISE_CIPHER_INVALID", "AKMP_INVALID", "UNSUPP_RSN_IE_VERSION", "INVALID_RSN_IE_CAP", "802_1X_AUTH_FAILED", "CIPHER_SUITE_REJECTED", "BEACON_TIMEOUT", "NO_AP_FOUND", "AUTH_FAIL", "ASSOC_FAIL", "HANDSHAKE_TIMEOUT" };
#define reason2str(r) ((r>176)?system_event_reasons[r-176]:system_event_reasons[r-1])

static esp_err_t _eventCallback(void *arg, system_event_t *event)
{
    if(event->event_id == SYSTEM_EVENT_SCAN_DONE) {
        MOLMC_LOGD(TAG, "SYSTEM_EVENT_SCAN_DONE");
        _scanDoneCb();
    }
    else if(event->event_id == SYSTEM_EVENT_STA_DISCONNECTED) {
        uint8_t reason = event->event_info.disconnected.reason;
        MOLMC_LOGD(TAG, "SYSTEM_EVENT_STA_DISCONNECTED reason = %u - %s", reason, reason2str(reason));
        if(reason == WIFI_REASON_NO_AP_FOUND) {
        } else if(reason == WIFI_REASON_AUTH_FAIL || reason == WIFI_REASON_ASSOC_FAIL) {
        } else if(reason == WIFI_REASON_BEACON_TIMEOUT || reason == WIFI_REASON_HANDSHAKE_TIMEOUT) {
        } else if(reason == WIFI_REASON_AUTH_EXPIRE) {
            MOLMC_LOGD(TAG, "reconnect");
            if(esp32_getAutoReconnect()) {
                esp32_begin();
            }
        } else {
        }
        HAL_NET_notify_disconnected();
    }
    else if(event->event_id == SYSTEM_EVENT_STA_START) {
        //MOLMC_LOGD(TAG, "SYSTEM_EVENT_STA_START");
    }
    else if(event->event_id == SYSTEM_EVENT_STA_STOP) {
        //MOLMC_LOGD(TAG, "SYSTEM_EVENT_STA_STOP");
    }
    else if(event->event_id == SYSTEM_EVENT_STA_GOT_IP) {
        MOLMC_LOGD(TAG, "SYSTEM_EVENT_STA_GOT_IP");
        /*
           tcpip_adapter_ip_info_t ip;
           tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip);
           MOLMC_LOGD(TAG, "%x", ip.ip.addr);
           */
        HAL_NET_notify_dhcp(true);
        HAL_NET_notify_connected();
    }
    return ESP_OK;
}

static void _network_event_task(void * arg){
    system_event_t *event = NULL;
    for (;;) {
        if(xQueueReceive(_network_event_queue, &event, 0) == pdTRUE){
            _eventCallback(NULL, event);
        } else {
            vTaskDelay(1);
        }
    }
    vTaskDelete(NULL);
    _network_event_task_handle = NULL;
}

static esp_err_t _network_event_cb(void *arg, system_event_t *event){
    if (xQueueSend(_network_event_queue, &event, portMAX_DELAY) != pdPASS) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

static void _start_network_event_task(){
    if(!_network_event_queue){
        _network_event_queue = xQueueCreate(32, sizeof(system_event_t *));
        if(!_network_event_queue){
            return;
        }
    }
    if(!_network_event_task_handle){
        xTaskCreatePinnedToCore(_network_event_task, "network_event", 4096, NULL, 2, &_network_event_task_handle, ARDUINO_RUNNING_CORE);
        if(!_network_event_task_handle){
            return;
        }
    }
    esp_event_loop_init(&_network_event_cb, NULL);
}

void tcpipInit(){
    static bool initialized = false;
    if(!initialized){
        initialized = true;
        _start_network_event_task();
        tcpip_adapter_init();
    }
}

static bool wifiLowLevelInit(){
    static bool lowLevelInitDone = false;
    if(!lowLevelInitDone){
        tcpipInit();
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
    //esp_wifi_deinit();
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
    if (cb != NULL) {
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

static bool _autoReconnect = false;
bool esp32_setAutoReconnect(bool autoReconnect)
{
    _autoReconnect = autoReconnect;
    return true;
}

bool esp32_getAutoReconnect(void)
{
    return _autoReconnect;
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

bool esp32_begin(void)
{
    if(!esp32_enableSTA(true)) {
        return false;
    }
    esp_wifi_start();
    esp_wifi_connect();
    esp32_setDHCP(true);
    return true;
}

static bool _smartConfigStarted = false;
static bool _smartConfigDone = false;

static void smartConfigCallback(uint32_t st, void* result)
{
    smartconfig_status_t status = (smartconfig_status_t) st;
    MOLMC_LOGD(TAG, "beginSmartConfig status = %d", status);
    if (status == SC_STATUS_LINK) {
        wifi_sta_config_t *sta_conf = reinterpret_cast<wifi_sta_config_t *>(result);
        MOLMC_LOGD(TAG, "ssid     = %s", sta_conf->ssid);
        MOLMC_LOGD(TAG, "password = %s", sta_conf->password);
        esp_wifi_set_config(WIFI_IF_STA, (wifi_config_t *)sta_conf);
        esp_wifi_connect();
    } else if (status == SC_STATUS_LINK_OVER) {
        _smartConfigDone = true;
    }
}

bool esp32_beginSmartConfig()
{
    MOLMC_LOGD(TAG, "esp32_beginSmartConfig");
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
    MOLMC_LOGD(TAG, "esp8266_stopSmartConfig");
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

