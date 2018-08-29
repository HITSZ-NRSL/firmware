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

#include "hw_config.h"
#include "esp8266_upgrade.h"
#include "esp8266_downfile.h"

const static char *TAG = "hal-download";

#define pheadbuffer "Connection: keep-alive\r\n\
Cache-Control: no-cache\r\n\
User-Agent: Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.101 Safari/537.36 \r\n\
Accept: */*\r\n\
Accept-Encoding: gzip,deflate\r\n\
Accept-Language: zh-CN,eb-US;q=0.8\r\n\r\n"

struct espconn *pespconn = NULL;
struct upgrade_server_info *upServer = NULL;

static ip_addr_t host_ip;
file_type_t filetype = OTA_APP_FILE;

os_timer_t downfile_timer;

down_status_t _downfile_status;
esp8266_downfile_handle_t downfile_handle = NULL;

char hostname_tmp[128] = {0};
char httppara_tmp[128] = {0};

void downfile_rsp(void *arg);
void downFile(char *hostname, char *httppara, char *md5para, void *check_cb);

//升级回调函数
void downfile_rsp(void *arg) {
    char output[64] = {0};
    struct upgrade_server_info *server = arg;

    memset(output, 0, sizeof(output));
    if(server->upgrade_flag == true) {
        MOLMC_LOGD(TAG, "downfile_success");
        _downfile_status = DOWNSTATUS_SUCCESS;
    } else {
        MOLMC_LOGD(TAG, "downfile_failed");
        _downfile_status = DOWNSTATUS_FAIL;
    }

    free(server->url);
    server->url = NULL;
    free(server);
    server = NULL;
}

//通过host name回去ip回调函数
void upServer_dns_found(const char *name, ip_addr_t *ipaddr, void *arg){
    struct espconn *pespconn = (struct espconn *) arg;

    free(pespconn);
    MOLMC_LOGD(TAG, "upServer_dns_found");
    if(ipaddr == NULL) {
        downfile_rsp(upServer);
        return;
    }
    memcpy(upServer->ip, &ipaddr->addr, 4);
    system_upgrade_start(upServer);
}

void downFile(char *hostname, char *httppara, char *md5para, void *check_cb){
    upServer = (struct upgrade_server_info *)os_zalloc(sizeof(struct upgrade_server_info));

    upServer->upgrade_version[5] = '\0';
    upServer->port = 80;
    upServer->check_cb = check_cb;
    upServer->check_times = 20000;

    if(upServer->url == NULL) {
        upServer->url = (uint8 *)os_zalloc(1024);
    }

    sprintf((char *)upServer->url, "GET %s HTTP/1.0\r\nHost: %s\r\n"pheadbuffer"", httppara, hostname);

    strcpy(upServer->md5, md5para);
    MOLMC_LOGD(TAG, "upServer->url=%s",upServer->url);
    host_ip.addr = ipaddr_addr(hostname);
    if (host_ip.addr != IPADDR_NONE) {
        memcpy(upServer->ip, &host_ip.addr, 4);
        system_upgrade_start(upServer);
    } else {
        pespconn = (struct espconn *)os_zalloc(sizeof(struct espconn));
        espconn_gethostbyname(pespconn, hostname, &host_ip, upServer_dns_found);
    }
}

down_status_t esp8266_downfile(const char *host, const char *uri, const char * md5, file_type_t type)
{
    MOLMC_LOGD(TAG, "host : %s, uri : %s", host, uri);
    filetype = type;
    downFile((char *)host, (char *)uri, (char *)md5, downfile_rsp);
    _downfile_status = DOWNSTATUS_DOWNING;
    return _downfile_status;
}

down_status_t esp8266_getDownfileStatus(void)
{
    optimistic_yield(10000);
    return _downfile_status;
}

void esp8266_set_downfile_handle(esp8266_downfile_handle_t fn)
{
    downfile_handle = fn;
}
