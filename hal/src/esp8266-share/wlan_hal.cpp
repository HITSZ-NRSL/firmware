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

#include "hw_config.h"
#include "wlan_hal.h"
#include "core_esp8266_wifi_generic.h"
#include "flash_map.h"
#include "memory_hal.h"
#include "macaddr_hal.h"
#include "delay_hal.h"

#define STATION_IF      0x00
#define SOFTAP_IF       0x01

static volatile uint32_t wlan_timeout_start;
static volatile uint32_t wlan_timeout_duration;

inline void WLAN_TIMEOUT(uint32_t dur) {
    wlan_timeout_start = HAL_Timer_Get_Milli_Seconds();
    wlan_timeout_duration = dur;
    //DEBUG("WLAN WD Set %d",(dur));
}
inline bool IS_WLAN_TIMEOUT() {
    return wlan_timeout_duration && ((HAL_Timer_Get_Milli_Seconds()-wlan_timeout_start)>wlan_timeout_duration);
}

inline void CLR_WLAN_TIMEOUT() {
    wlan_timeout_duration = 0;
    //DEBUG("WLAN WD Cleared, was %d", wlan_timeout_duration);
}

//=======net notify===========
static HAL_NET_Callbacks netCallbacks = { 0 };

void HAL_NET_SetCallbacks(const HAL_NET_Callbacks* callbacks, void* reserved)
{
    netCallbacks.notify_connected = callbacks->notify_connected;
    netCallbacks.notify_disconnected = callbacks->notify_disconnected;
    netCallbacks.notify_dhcp = callbacks->notify_dhcp;
    netCallbacks.notify_can_shutdown = callbacks->notify_can_shutdown;
}

void HAL_NET_notify_connected()
{
    if (netCallbacks.notify_connected) {
        netCallbacks.notify_connected();
    }
}

void HAL_NET_notify_disconnected()
{
    if (netCallbacks.notify_disconnected) {
        netCallbacks.notify_disconnected();
    }
}

void HAL_NET_notify_dhcp(bool dhcp)
{
    if (netCallbacks.notify_dhcp) {
        netCallbacks.notify_dhcp(dhcp); // dhcp dhcp
    }
}

void HAL_NET_notify_can_shutdown()
{
    if (netCallbacks.notify_can_shutdown) {
        netCallbacks.notify_can_shutdown();
    }
}

uint32_t HAL_NET_SetNetWatchDog(uint32_t timeOutInMS)
{
    return 0;
}

//=======wifi activate/deactivate===========
wlan_result_t wlan_activate()
{
    return 0;
}

wlan_result_t wlan_deactivate()
{
    return 0;
}

void wlan_setup()
{
    esp8266_wifiInit();
    esp8266_setMode(WIFI_STA);
    esp8266_setDHCP(true);
    esp8266_setAutoConnect(true);
    esp8266_setAutoReconnect(true);
}

//=======wifi connect===========
int wlan_connect_init()
{
    return esp8266_connect();
}

wlan_result_t wlan_connect_finalize()
{
    return 0;
}

wlan_result_t wlan_disconnect_now()
{
    return esp8266_disconnect();
}

void wlan_connect_cancel(bool called_from_isr)
{

}

int wlan_connected_rssi(void)
{
    return esp8266_getRSSI();
}

void wlan_drive_now(void)
{
}

//================credentials======================
int wlan_clear_credentials()
{
    return 0;
}

int wlan_has_credentials()
{
    return 0;
}

bool wlan_reset_credentials_store_required()
{
    return false;
}

wlan_result_t wlan_reset_credentials_store()
{
    wlan_clear_credentials();
    return 0;
}

int wlan_get_credentials(wlan_scan_result_t callback, void* callback_data)
{
    return 0;
}

int wlan_set_credentials(WLanCredentials* c)
{
    struct station_config conf;
    strcpy((char*)(conf.ssid), c->ssid);

    if(c->password) {
        if (strlen(c->password) == 64) // it's not a passphrase, is the PSK
            memcpy((char*)(conf.password), c->password, 64);
        else
            strcpy((char*)(conf.password), c->password);
    } else {
        *conf.password = 0;
    }
    conf.bssid_set = 0;
    ETS_UART_INTR_DISABLE();
    // workaround for #1997: make sure the value of ap_number is updated and written to flash
    // to be removed after SDK update
    wifi_station_ap_number_set(2);
    wifi_station_ap_number_set(1);

    wifi_station_set_config(&conf);
    ETS_UART_INTR_ENABLE();
    return 0;
}

//==============imlink==================
void wlan_Imlink_start()
{
    esp8266_beginSmartConfig();
}

imlink_status_t wlan_Imlink_get_status()
{
    if(!esp8266_smartConfigDone()) {
        return IMLINK_DOING;
    } else {
        return IMLINK_SUCCESS;
    }
}

void wlan_Imlink_stop()
{
    esp8266_stopSmartConfig();
}

void wlan_fetch_ipconfig(WLanConfig* config)
{
    memset(config, 0, sizeof(WLanConfig));
    config->size = sizeof(WLanConfig);

    struct ip_info ip;
    wifi_get_ip_info(STATION_IF, &ip);
    config->nw.aucIP.ipv4 = ip.ip.addr;
    config->nw.aucSubnetMask.ipv4 = ip.netmask.addr;
    config->nw.aucDefaultGateway.ipv4 = ip.gw.addr;

    ip_addr_t dns_ip = dns_getserver(0);
    config->nw.aucDNSServer.ipv4 = dns_ip.addr;
    wifi_get_macaddr(STATION_IF, config->nw.uaMacAddr);

    struct station_config conf;
    wifi_station_get_config(&conf);
    memcpy(config->uaSSID, conf.ssid, 32);
    memcpy(config->BSSID, conf.bssid, 6);
}

void wlan_set_error_count(uint32_t errorCount)
{
}

/**
 * Sets the IP source - static or dynamic.
 */
void wlan_set_ipaddress_source(IPAddressSource source, bool persist, void* reserved)
{
}

/**
 * Sets the IP Addresses to use when the device is in static IP mode.
 * @param device
 * @param netmask
 * @param gateway
 * @param dns1
 * @param dns2
 * @param reserved
 */
void wlan_set_ipaddress(const HAL_IPAddress* device, const HAL_IPAddress* netmask,
        const HAL_IPAddress* gateway, const HAL_IPAddress* dns1, const HAL_IPAddress* dns2, void* reserved)
{

}

static WLanSecurityType toSecurityType(AUTH_MODE authmode)
{
    switch(authmode)
    {
        case AUTH_OPEN:
            return WLAN_SEC_UNSEC;
            break;
        case AUTH_WEP:
            return WLAN_SEC_WEP;
            break;
        case AUTH_WPA_PSK:
            return WLAN_SEC_WPA;
            break;
        case AUTH_WPA2_PSK:
        case AUTH_WPA_WPA2_PSK:
            return WLAN_SEC_WPA2;
            break;
        case AUTH_MAX:
            return WLAN_SEC_NOT_SET;
            break;
    }
}

static WLanSecurityCipher toCipherType(AUTH_MODE authmode)
{
    switch(authmode)
    {
        case AUTH_WEP:
            return WLAN_CIPHER_AES;
            break;
        case AUTH_WPA_PSK:
            return WLAN_CIPHER_TKIP;
            break;
        default:
            break;
    }
    return WLAN_CIPHER_NOT_SET;
}

struct WlanScanInfo
{
    wlan_scan_result_t callback;
    void* callback_data;
    int count;
    bool completed;
};

struct WlanApSimple
{
   uint8_t bssid[6];
   int rssi;
};

WlanScanInfo scanInfo;
void scan_done_cb(void *arg, STATUS status)
{
    WiFiAccessPoint data;
    WlanApSimple apSimple;
    int n = 0, m = 0, j = 0;
    bss_info *it = (bss_info*)arg;

    if(status == OK) {
        //获取ap数量
        for(n = 0; it; it = STAILQ_NEXT(it, next), n++);
        scanInfo.count = n;

        //申请内存
        WlanApSimple *pNode = (WlanApSimple *)malloc(sizeof(struct WlanApSimple)*scanInfo.count);
        if(pNode == NULL) {
            scanInfo.completed = true;
            return;
        }

        for(n = 0, it = (bss_info*)arg; it; it = STAILQ_NEXT(it, next), n++)
        {
            memcpy(pNode[n].bssid, it->bssid, 6);
            pNode[n].rssi = it->rssi;
        }

        //根据rssi排序
        for(n = 0; n < scanInfo.count - 1; n++)
        {
            j = n;
            for(m = n+1; m < scanInfo.count; m++)
            {
                if(pNode[m].rssi > pNode[j].rssi) {
                    j = m;
                }
            }
            if(j != n) {
                memcpy(&apSimple, &pNode[n], sizeof(struct WlanApSimple));
                memcpy(&pNode[n], &pNode[j], sizeof(struct WlanApSimple));
                memcpy(&pNode[j], &apSimple, sizeof(struct WlanApSimple));
            }
        }

        //填充ap 列表
        for(n = 0; n < scanInfo.count; n++)
        {
            for(it = (bss_info*)arg; it; it = STAILQ_NEXT(it, next))
            {
                if(!memcmp(pNode[n].bssid, it->bssid, 6)) {
                    memset(&data, 0, sizeof(WiFiAccessPoint));
                    memcpy(data.ssid, it->ssid, it->ssid_len);
                    data.ssidLength = it->ssid_len;
                    memcpy(data.bssid, it->bssid, 6);
                    data.security = toSecurityType(it->authmode);
                    data.cipher = toCipherType(it->authmode);
                    data.channel = it->channel;
                    data.rssi = it->rssi;
                    scanInfo.callback(&data, scanInfo.callback_data);
                    break;
                }
            }
        }
        scanInfo.completed = true;
        free(pNode);
    }
}

extern "C" {
void optimistic_yield(uint32_t interval_us);
}

int wlan_scan(wlan_scan_result_t callback, void* cookie)
{
    esp8266_setMode(WIFI_STA);
    memset((void *)&scanInfo, 0, sizeof(struct WlanScanInfo));
    scanInfo.callback = callback;
    scanInfo.callback_data = cookie;
    scanInfo.count = 0;
    scanInfo.completed = false;
    if(wifi_station_scan(NULL, scan_done_cb)) {
        WLAN_TIMEOUT(6000);
        while(!scanInfo.completed)
        {
            optimistic_yield(100);
            if(IS_WLAN_TIMEOUT()) {
                CLR_WLAN_TIMEOUT();
                break;
            }
        }
        return scanInfo.count;
    } else {
        return -1;
    }
}

/**
 * wifi set station and ap mac addr
 */
int wlan_set_macaddr(uint8_t *stamacaddr, uint8_t *apmacaddr)
{
    if (stamacaddr != NULL && apmacaddr != NULL) {
        mac_param_t mac_addrs;
        mac_addrs.header = FLASH_MAC_HEADER;

        memset(mac_addrs.stamac_addrs, 0, sizeof(mac_addrs.stamac_addrs));
        memcpy(mac_addrs.stamac_addrs, stamacaddr, sizeof(mac_addrs.stamac_addrs));

        memset(mac_addrs.apmac_addrs, 0, sizeof(mac_addrs.apmac_addrs));
        memcpy(mac_addrs.apmac_addrs, apmacaddr, sizeof(mac_addrs.apmac_addrs));

        uint32_t len = sizeof(mac_addrs);
        HAL_FLASH_Interminal_Erase(HAL_FLASH_Interminal_Get_Sector(FLASH_MAC_START_ADDR));
        HAL_FLASH_Interminal_Write(FLASH_MAC_START_ADDR, (uint32_t *)&mac_addrs, len);
        return 0;
    }
    return -1;
}

/**
 * wifi get station and ap mac addr
 */
int wlan_get_macaddr(uint8_t *stamacaddr, uint8_t *apmacaddr)
{
    if(!wifi_get_macaddr(STATION_IF, stamacaddr)) {
        return -1;
    }

    if(!wifi_get_macaddr(SOFTAP_IF, apmacaddr)) {
        return -1;
    }
    return 0;
}

int wlan_set_macaddr_from_flash(uint8_t *stamacaddr, uint8_t *apmacaddr)
{
    if(!wifi_set_macaddr(STATION_IF, stamacaddr)) {
        return -1;
    }

    if(!wifi_set_macaddr(SOFTAP_IF, apmacaddr)) {
        return -1;
    }
    return 0;
}

int wlan_set_macaddr_when_init(void)
{
    mac_param_t mac_addrs;
    HAL_FLASH_Interminal_Read(FLASH_MAC_START_ADDR, (uint32_t *)&mac_addrs, sizeof(mac_addrs));
    if (FLASH_MAC_HEADER == mac_addrs.header){
        esp8266_setMode(WIFI_AP_STA);
        wlan_set_macaddr_from_flash(mac_addrs.stamac_addrs, mac_addrs.apmac_addrs);
        esp8266_setMode(WIFI_STA);
    }
}

