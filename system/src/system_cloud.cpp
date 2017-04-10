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

#include <stdlib.h>
#include <stdio.h>

#include "hw_config.h"
#include "core_hal.h"
#include "params_hal.h"
#include "ota_flash_hal.h"
#include "wiring_string.h"
#include "wiring_tcpclient.h"
#include "wiring_udp.h"
#include "wiring_wifi.h"
#include "wiring_cellular.h"
#include "system_mqttclient.h"
#include "system_cloud_def.h"
#include "system_datapoint.h"
#include "system_cloud.h"
#include "system_mode.h"
#include "system_task.h"
#include "system_threading.h"
#include "system_update.h"
#include "system_rgbled.h"
#include "wiring_time.h"
#include "wiring_ticks.h"
#include "wiring_random.h"
#include "wiring_httpclient.h"
#include "system_product.h"
#include "system_update.h"
#include "system_version.h"
#include "ajson.h"

/*debug switch*/
#define SYSTEM_CLOUD_DEBUG

#ifdef SYSTEM_CLOUD_DEBUG
#define SCLOUD_DEBUG(...)  do {DEBUG(__VA_ARGS__);}while(0)
#define SCLOUD_DEBUG_D(...)  do {DEBUG_D(__VA_ARGS__);}while(0)
#else
#define SCLOUD_DEBUG(...)
#define SCLOUD_DEBUG_D(...)
#endif

using namespace intorobot;

#ifndef configNO_CLOUD
volatile uint8_t g_intorobot_network_connected = 0;    //网络连接状态 1连接 0断开
volatile uint8_t g_intorobot_cloud_pregrocessed = 0;   //平台链接预处理状态 1已经处理
volatile uint8_t g_intorobot_cloud_connected = 0;      //平台连接状态 1连接上了

struct CallBackList g_callback_list;  //回调结构体
struct CloudDebugBuffer  g_debug_tx_buffer;
struct CloudDebugBuffer  g_debug_rx_buffer;

TCPClient g_mqtt_tcp_client;
MqttClientClass g_mqtt_client;
RGBLEDState led_state;


pCallBack get_subscribe_callback(char * fulltopic);
WidgetBaseClass *get_widget_subscribe_callback(char * fulltopic);
void add_subscribe_callback(api_version_t version, char *topic, char *device_id, void (*callback)(uint8_t*, uint32_t), uint8_t qos);
void add_widget_subscibe_callback(api_version_t version, char *topic, char *device_id, WidgetBaseClass *pWidgetBase, uint8_t qos);
void del_subscribe_callback(api_version_t version, char * topic, char *device_id);
void mqtt_receive_debug_info(uint8_t *pIn, uint32_t len);

void mqtt_client_callback(char *topic, uint8_t *payload, uint32_t length)
{
    SCLOUD_DEBUG("mqtt callback!");
    SCLOUD_DEBUG("topic: %s", topic);

    uint8_t *pdata = NULL;
    pCallBack pcallback=get_subscribe_callback(topic);
    if(pcallback!=NULL) {
        pdata = (uint8_t *)malloc(length+1);
        memset(pdata, 0, length+1);
        memcpy(pdata, payload, length);
        SCLOUD_DEBUG("data: %s", pdata);
        pcallback(pdata,length);
        free(pdata);
    }

    WidgetBaseClass *pwidgetbase=get_widget_subscribe_callback(topic);
    if(pwidgetbase!=NULL) {
        pdata = (uint8_t *)malloc(length+1);
        memset(pdata, 0, length+1);
        memcpy(pdata, payload, length);
        SCLOUD_DEBUG("data: %s", pdata);
        pwidgetbase->widgetBaseCallBack(pdata,length);
        free(pdata);
    }
}

typedef enum {
    UPGRADE_REPLY_READY = 0,
    UPGRADE_REPLY_PROGRESS,
    UPGRADE_REPLY_DOWN_FAIL,
    UPGRADE_REPLY_DOWN_SUCC,
    UPGRADE_REPLY_UPDATE_FAIL,
    UPGRADE_REPLY_UPDATE_SUCC,
    UPGRADE_REPLY_DOWN_SUCC_EXIT,
    UPGRADE_REPLY_TYPEEEOR,
    UPGRADE_REPLY_REBOOT_READY
} upgrade_reply_t;

static void send_ota_status_v1(const upgrade_reply_t upgrade_reply, const uint8_t progress)
{
    String status = "";
    switch(upgrade_reply)
    {
        case UPGRADE_REPLY_DOWN_FAIL:
            status += String(INTOROBOT_MQTT_RESPONSE_OTA_DOWN_FAIL);
            break;
        case UPGRADE_REPLY_DOWN_SUCC:
            status += String(INTOROBOT_MQTT_RESPONSE_OTA_DOWN_SUCC);
            break;
        case UPGRADE_REPLY_UPDATE_FAIL:
            status += String(INTOROBOT_MQTT_RESPONSE_OTA_UPDATE_FAIL);
            break;
        case UPGRADE_REPLY_UPDATE_SUCC:
            status += String(INTOROBOT_MQTT_RESPONSE_OTA_UPDATE_SUCC);
            break;
        case UPGRADE_REPLY_DOWN_SUCC_EXIT:
            status += String(INTOROBOT_MQTT_RESPONSE_OTA_DOWN_SUCC_EXIT);
            break;
        case UPGRADE_REPLY_TYPEEEOR:
            status += String(INTOROBOT_MQTT_RESPONSE_OTA_TYPEEEOR);
            break;
        case UPGRADE_REPLY_REBOOT_READY:
            status += String(INTOROBOT_MQTT_RESPONSE_REBOOT_READY);
            break;
        case UPGRADE_REPLY_READY:
        default:
            status += String(INTOROBOT_MQTT_RESPONSE_OTA_READY);
            break;
    }
    intorobot_publish(API_VERSION_V1, INTOROBOT_MQTT_RESPONSE_TOPIC, (uint8_t*)status.c_str(), status.length(), 0, false);
}

static void send_subsys_status_v1(const upgrade_reply_t upgrade_reply, const uint8_t progress)
{
    String status = "{\"status\":\"";
    switch(upgrade_reply)
    {
        case UPGRADE_REPLY_PROGRESS:
            status += String(INTOROBOT_MQTT_RESPONSE_SUBSYS_READY_PROGRESS) + "\",\"progress\":" + String(progress) + "}";
            break;
        case UPGRADE_REPLY_DOWN_FAIL:
            status += String(INTOROBOT_MQTT_RESPONSE_SUBSYS_DOWN_FAIL) + "\"}";
            break;
        case UPGRADE_REPLY_DOWN_SUCC:
            status += String(INTOROBOT_MQTT_RESPONSE_SUBSYS_DOWN_SUCC) + "\"}";
            break;
        case UPGRADE_REPLY_UPDATE_FAIL:
            status += String(INTOROBOT_MQTT_RESPONSE_SUBSYS_UPDATE_FAIL) + "\"}";
            break;
        case UPGRADE_REPLY_UPDATE_SUCC:
            status += String(INTOROBOT_MQTT_RESPONSE_SUBSYS_UPDATE_SUCC) + "\"}";
            break;
        case UPGRADE_REPLY_DOWN_SUCC_EXIT:
            status += String(INTOROBOT_MQTT_RESPONSE_SUBSYS_DOWN_SUCC_EXIT) + "\"}";
            break;
        case UPGRADE_REPLY_TYPEEEOR:
            status += String(INTOROBOT_MQTT_RESPONSE_SUBSYS_TYPEEEOR) + "\"}";
            break;
        case UPGRADE_REPLY_READY:
        default:
            status += String(INTOROBOT_MQTT_RESPONSE_SUBSYS_READY_PROGRESS) + + "\"}";
            break;
    }
    intorobot_publish(API_VERSION_V1, INTOROBOT_MQTT_RESPONSE_JSON_TOPIC, (uint8_t*)status.c_str(), status.length(), 0, false);
}

void intorobot_send_subsys_progress(uint8_t progress)
{
    send_subsys_status_v1(UPGRADE_REPLY_PROGRESS, progress);
}

/***********************v1版本控制回调函数***********************/
static void ota_update_callback(uint8_t *payload, uint32_t len)
{
    SCLOUD_DEBUG("v1 : online update!");

    uint32_t n;
    char flag=0;
    String s_payload="", domain="", param="";
    aJsonClass aJson;

    for(n=0; n<len; n++)
    {s_payload+=(char)payload[n];}

    led_state.save();
    system_rgb_color(RGB_COLOR_YELLOW);

    send_ota_status_v1(UPGRADE_REPLY_READY, 0);

    aJsonObject *root = aJson.parse((char *)s_payload.c_str());
    if(root == NULL) {
        flag=1;
    }

    aJsonObject* type_Object = aJson.getObjectItem(root, "board");
    if(type_Object == NULL) {
        flag=1;
    } else {
        char board[32]="";
        system_platform_id(board);
        if(strcmp(type_Object->valuestring + 3, board + 3))
        {flag=2;}
    }

    aJsonObject* md5_Object = aJson.getObjectItem(root, "md5");
    if(md5_Object == NULL)
    {flag=1;}

    aJsonObject* dtoken_Object = aJson.getObjectItem(root, "dwn_token");
    if(dtoken_Object == NULL) {
        flag=1;
    }

    if(0==flag) {
        char down_domain[36]={0};
        HAL_PARAMS_Get_System_dw_domain(down_domain, sizeof(down_domain));
        if (strlen(down_domain)) {
            domain+=down_domain;
        } else {
            domain+=INTOROBOT_UPDATE_DOMAIN;
        }

        param+=String(INTOROBOT_OTA_UPDATE_URL) + "?dwn_token=" + String(dtoken_Object->valuestring);

        uint32_t size = 0;
        uint8_t down_status = 1, progress = 0;
#if PLATFORM_ID == PLATFORM_FIG
        HTTPUpdate httpUpdate;
        String url="http://" + domain + param;
        httpUpdate.setStoreStartAddress(HAL_OTA_FlashAddress());
        httpUpdate.setStoreMaxSize(HAL_OTA_FlashLength());
        t_httpUpdate_return ret = httpUpdate.update(url);
        switch(ret) {
            case HTTP_UPDATE_OK:
                SCLOUD_DEBUG("v2 :HTTP_UPDATE_OK!");
                size = httpUpdate.size();
                down_status = 0;
                break;
            default:
                SCLOUD_DEBUG("v2 :HTTP_UPDATE_FAIL!");
                down_status = 1;
                break;
        }
#else
        down_status_t status;
        down_status_t result = HAL_OTA_Download_App(domain.c_str(), param.c_str(), md5_Object->valuestring);
        switch(result)
        {
            case DOWNSTATUS_SUCCESS:
                break;
            case DOWNSTATUS_DOWNING:
                do {
                    status = HAL_OTA_Get_App_Download_Status();
                    if(DOWNSTATUS_SUCCESS == status) {
                        down_status = 0;
                        break;
                    } else if(DOWNSTATUS_DOWNING == status) {
                        progress = HAL_OTA_Get_Download_Progress();
                        delay(1000);
                    } else if(DOWNSTATUS_FAIL == status) {
                        down_status = 1;
                        break;
                    }
                }while(1);
                break;
            default:
                down_status = 1;
                break;
        }
#endif
        if(!down_status) {
            send_ota_status_v1(UPGRADE_REPLY_DOWN_SUCC_EXIT, 0);
            delay(500);
            HAL_OTA_Update_App(size);
            HAL_Core_System_Reset();
            while(1); //不会运行到地方
        } else {
            flag=1;
        } //下载失败
    }
    if (root != NULL)
    {aJson.deleteItem(root);}
    if (2==flag) {  // board type error
        send_ota_status_v1(UPGRADE_REPLY_TYPEEEOR, 0);
    } else {//download fall
        send_ota_status_v1(UPGRADE_REPLY_DOWN_FAIL, 0);
    }
    led_state.restore();
}

static void subsys_update_callback(uint8_t *payload, uint32_t len)
{
    SCLOUD_DEBUG("v1 : subsys update!");

    uint32_t n;
    String s_payload="", domain="", param="";
    aJsonClass aJson;
    char flag=0,result=1;
    bool board_type_error = false;

    for(n=0; n<len; n++)
    {s_payload+=(char)payload[n];}

    led_state.save();
    system_rgb_color(RGB_COLOR_YELLOW);

    send_subsys_status_v1(UPGRADE_REPLY_READY, 0);

    aJsonObject *root = aJson.parse((char *)s_payload.c_str());
    if(root == NULL) {
        flag=1;
    }

    aJsonObject* sys_Ver_Object = aJson.getObjectItem(root, "sys_ver");
    if(sys_Ver_Object == NULL) {
        flag=1;
    } else {
        char sys_ver[24]={0};
        HAL_PARAMS_Get_System_subsys_ver(sys_ver, sizeof(sys_ver));
        if(!strcmp(sys_ver, sys_Ver_Object->valuestring))
        {flag=3;}
    }

    if(0==flag) {
        char down_domain[36]={0};
        HAL_PARAMS_Get_System_dw_domain(down_domain, sizeof(down_domain));
        if (strlen(down_domain)) {
            domain+=down_domain;
        } else {
            domain+=INTOROBOT_UPDATE_DOMAIN;
        }
        char name[24]={0};
        system_platform_name(name);

        param+="/downloads/" + String(name) + "/" + String(sys_Ver_Object->valuestring);

        uint8_t down_status = 1, progress = 0;
        uint32_t defAppSize = 0, bootSize = 0;
#if PLATFORM_ID == PLATFORM_FIG
        HTTPUpdate httpUpdate;
        String url="http://" + domain + param + "/fig-boot.bin";
        httpUpdate.setStoreStartAddress(HAL_BOOT_FlashAddress());
        httpUpdate.setStoreMaxSize(HAL_BOOT_FlashLength());
        t_httpUpdate_return ret = httpUpdate.update(url);
        if(HTTP_UPDATE_OK == ret) {
            bootSize = httpUpdate.size();
            url="http://" + domain + param + "/default-fig.bin";
            httpUpdate.setStoreStartAddress(HAL_DEF_APP_FlashAddress());
            httpUpdate.setStoreMaxSize(HAL_DEF_APP_FlashLength());
            httpUpdate.setSendProgressCb(intorobot_send_subsys_progress);
            t_httpUpdate_return ret = httpUpdate.update(url);
            if(HTTP_UPDATE_OK == ret) {
                defAppSize = httpUpdate.size();
                SCLOUD_DEBUG("v2 :HTTP_UPDATE_OK!");
                down_status = 0;
            }
        }
#else
        down_status_t status;
        down_status_t result = HAL_OTA_Download_Subsys(domain.c_str(), param.c_str());
        switch(result) {
            case DOWNSTATUS_SUCCESS:
                break;
            case DOWNSTATUS_DOWNING:
                do {
                    status = HAL_OTA_Get_Subsys_Download_Status();
                    if(DOWNSTATUS_SUCCESS == status) {
                        down_status = 0;
                        break;
                    } else if(DOWNSTATUS_DOWNING == status) {
                        progress = HAL_OTA_Get_Download_Progress();
                        send_subsys_status_v1(UPGRADE_REPLY_PROGRESS, progress);
                        delay(1000);
                    } else if(DOWNSTATUS_FAIL == status) {
                        down_status = 1;
                        break;
                    }
                }while(1);
                break;
            default:
                down_status = 1;
                break;
        }
#endif
        if(!down_status) {
            send_subsys_status_v1(UPGRADE_REPLY_DOWN_SUCC_EXIT, 0);
            delay(500);
            HAL_OTA_Upadate_Subsys(defAppSize, bootSize, true);
            HAL_Core_System_Reset();
            while(1); //不会运行到地方
        } else {
            flag=1;
        } //下载失败
    }

    if (root != NULL)
    {aJson.deleteItem(root);}

    if (1==flag) {        //download fall
        send_subsys_status_v1(UPGRADE_REPLY_DOWN_FAIL, 0);
    } else if (2==flag) { // board type error
        send_subsys_status_v1(UPGRADE_REPLY_TYPEEEOR, 0);
    } else if (3==flag) { //download fall
        send_subsys_status_v1(UPGRADE_REPLY_DOWN_SUCC_EXIT, 0);
    }
    led_state.restore();
}

static void system_reboot_callback(uint8_t *payload, uint32_t len)
{
    SCLOUD_DEBUG("system reboot!");
    //system reset ready
    intorobot_publish(API_VERSION_V1, INTOROBOT_MQTT_RESPONSE_TOPIC, (uint8_t *)INTOROBOT_MQTT_RESPONSE_REBOOT_SUCC, strlen(INTOROBOT_MQTT_RESPONSE_REBOOT_SUCC), 0, false);
    HAL_Core_System_Reset();
}

static void system_debug_callback(uint8_t *payload, uint32_t len)
{
    SCLOUD_DEBUG("system debug recieve!");
    mqtt_receive_debug_info(payload, len);
}

/****************************************************************/

/***********************v2版本控制回调函数***********************/
static void send_upgrade_status(const upgrade_reply_t upgrade_reply, const uint8_t progress)
{
    String status = "{\"status\":\"";
    switch(upgrade_reply)
    {
        case UPGRADE_REPLY_PROGRESS:
            status += String(INTOROBOT_MQTT_REPLY_READY_PROGRESS) + "\",\"progress\":" + String(progress) + "}";
            break;
        case UPGRADE_REPLY_DOWN_FAIL:
            status += String(INTOROBOT_MQTT_REPLY_DOWN_FAIL) + "\"}";
            break;
        case UPGRADE_REPLY_DOWN_SUCC:
            status += String(INTOROBOT_MQTT_REPLY_DOWN_SUCC) + "\"}";
            break;
        case UPGRADE_REPLY_UPDATE_FAIL:
            status += String(INTOROBOT_MQTT_REPLY_UPDATE_FAIL) + "\"}";
            break;
        case UPGRADE_REPLY_UPDATE_SUCC:
            status += String(INTOROBOT_MQTT_REPLY_UPDATE_SUCC) + "\"}";
            break;
        case UPGRADE_REPLY_DOWN_SUCC_EXIT:
            status += String(INTOROBOT_MQTT_REPLY_DOWN_SUCC_EXIT) + "\"}";
            break;
        case UPGRADE_REPLY_TYPEEEOR:
            status += String(INTOROBOT_MQTT_REPLY_TYPEEEOR) + "\"}";
            break;
        case UPGRADE_REPLY_REBOOT_READY:
            status += String(INTOROBOT_MQTT_REPLY_REBOOT_READY) + "\"}";
            break;
        case UPGRADE_REPLY_READY:
        default:
            status += String(INTOROBOT_MQTT_REPLY_READY_PROGRESS) + "\"}";
            break;
    }
    intorobot_publish(API_VERSION_V2, INTOROBOT_MQTT_REPLY_TOPIC, (uint8_t*)status.c_str(), status.length(), 0, false);
}

void intorobot_send_upgrade_progress(uint8_t progress)
{
    send_upgrade_status(UPGRADE_REPLY_PROGRESS, progress);
}

//在线编程升级
static void intorobot_ota_upgrade(const char *token, const char *md5)
{
    SCLOUD_DEBUG("v2 :online upgrade!");

    bool flag = false;
    String domain="", param="";
    uint8_t progress = 0;

    led_state.save();
    system_rgb_color(RGB_COLOR_YELLOW);

    send_upgrade_status(UPGRADE_REPLY_READY, 0);

    char down_domain[36]={0};
    HAL_PARAMS_Get_System_dw_domain(down_domain, sizeof(down_domain));
    if (strlen(down_domain)) {
        domain+=down_domain;
    } else {
        domain+=INTOROBOT_UPDATE_DOMAIN;
    }

    param += String(INTOROBOT_OTA_UPDATE_URL) + "?dwn_token=" + String(token);

    uint32_t size = 0;
#if PLATFORM_ID == PLATFORM_FIG
    HTTPUpdate httpUpdate;
    String url="http://" + domain + param;
    httpUpdate.setStoreStartAddress(HAL_OTA_FlashAddress());
    httpUpdate.setStoreMaxSize(HAL_OTA_FlashLength());
    httpUpdate.setSendProgressCb(intorobot_send_upgrade_progress);
    t_httpUpdate_return ret = httpUpdate.update(url);
    switch(ret) {
        case HTTP_UPDATE_OK:
            SCLOUD_DEBUG("v2 :HTTP_UPDATE_OK!");
            size = httpUpdate.size();
            flag = true;
            break;
        default:
            SCLOUD_DEBUG("v2 :HTTP_UPDATE_FAIL!");
            break;
    }
#else
    down_status_t status;
    down_status_t result = HAL_OTA_Download_App(domain.c_str(), param.c_str(), md5);
    switch(result)
    {
        case DOWNSTATUS_SUCCESS:
            flag = true;
            break;
        case DOWNSTATUS_DOWNING:
            do {
                status = HAL_OTA_Get_App_Download_Status();
                if(DOWNSTATUS_SUCCESS == status) {
                    flag = true;
                    break;
                } else if(DOWNSTATUS_DOWNING == status) {
                    progress = HAL_OTA_Get_Download_Progress();
                    send_upgrade_status(UPGRADE_REPLY_PROGRESS, progress);
                    delay(1000);
                } else if(DOWNSTATUS_FAIL == status) {
                    break;
                }
            } while(1);
            break;
        default:
            break;
    }
#endif
    if(flag) {
        send_upgrade_status(UPGRADE_REPLY_DOWN_SUCC_EXIT, 0);
        delay(500);
        HAL_OTA_Update_App(size);
        HAL_Core_System_Reset();
        while(1); //不会运行到地方
    } else {
        send_upgrade_status(UPGRADE_REPLY_DOWN_FAIL, 0);
    }
    led_state.restore();
}

// 子系统升级
static void intorobot_subsys_upgrade(const char *version)
{
    SCLOUD_DEBUG("v2 :subsys_upgrade!");

    bool flag = false;
    String domain="", param="";
    uint8_t progress = 0;

    led_state.save();
    system_rgb_color(RGB_COLOR_YELLOW);

    send_upgrade_status(UPGRADE_REPLY_READY, 0);

    char down_domain[36]={0};
    HAL_PARAMS_Get_System_dw_domain(down_domain, sizeof(down_domain));
    if (strlen(down_domain)) {
        domain+=down_domain;
    } else {
        domain+=INTOROBOT_UPDATE_DOMAIN;
    }
    char name[24]={0};
    system_platform_name(name);
    param+="/downloads/" + String(name) + "/" + String(version);

    uint32_t defAppSize = 0, bootSize = 0;
    down_status_t status;
#if PLATFORM_ID == PLATFORM_FIG
    HTTPUpdate httpUpdate;
    String url="http://" + domain + param + "/fig-boot.bin";
    httpUpdate.setStoreStartAddress(HAL_BOOT_FlashAddress());
    httpUpdate.setStoreMaxSize(HAL_BOOT_FlashLength());
    t_httpUpdate_return ret = httpUpdate.update(url);
    if(HTTP_UPDATE_OK == ret) {
        bootSize = httpUpdate.size();
        url="http://" + domain + param + "/default-fig.bin";
        httpUpdate.setStoreStartAddress(HAL_DEF_APP_FlashAddress());
        httpUpdate.setStoreMaxSize(HAL_DEF_APP_FlashLength());
        httpUpdate.setSendProgressCb(intorobot_send_upgrade_progress);
        t_httpUpdate_return ret = httpUpdate.update(url);
        if(HTTP_UPDATE_OK == ret) {
            defAppSize = httpUpdate.size();
            SCLOUD_DEBUG("v2 :HTTP_UPDATE_OK!");
            flag = true;
        }
    }
#else
    down_status_t result = HAL_OTA_Download_Subsys(domain.c_str(), param.c_str());
    switch(result)
    {
        case DOWNSTATUS_SUCCESS:
            break;
        case DOWNSTATUS_DOWNING:
            do {
                status = HAL_OTA_Get_Subsys_Download_Status();
                if(DOWNSTATUS_SUCCESS == status) {
                    break;
                } else if(DOWNSTATUS_DOWNING == status) {
                    progress = HAL_OTA_Get_Download_Progress();
                    send_upgrade_status(UPGRADE_REPLY_PROGRESS, progress);
                    delay(1000);
                    flag = true;
                } else if(DOWNSTATUS_FAIL == status) {
                    break;
                }
            }while(1);
            break;
        default:
            break;
    }
#endif
    if(flag) {
        send_upgrade_status(UPGRADE_REPLY_DOWN_SUCC_EXIT, 0);
        delay(500);
        HAL_OTA_Upadate_Subsys(defAppSize, bootSize, true);
        HAL_Core_System_Reset();
        while(1); //不会运行到地方
    } else {
        send_upgrade_status(UPGRADE_REPLY_DOWN_FAIL, 0);
    }
    led_state.restore();
}

//固件升级
static void intorobot_firmware_upgrade(const char *version)
{

}

void cloud_action_callback(uint8_t *payload, uint32_t len)
{
    SCLOUD_DEBUG("v2 :cloud_action_callback!");

    aJsonClass aJson;
    String s_payload;
    aJsonObject *root=NULL, *boardObject=NULL, *cmdObject=NULL, *dtokenObject=NULL, *versionObject=NULL;

    for(int n=0; n < len; n++)
    {
        s_payload+=(char)payload[n];
    }

    root = aJson.parse((char *)s_payload.c_str());
    if(root == NULL) {
        goto finish;
    }

    boardObject = aJson.getObjectItem(root, "board");
    if(boardObject == NULL){
        goto finish;
    } else {
        char board[32]="";
        system_platform_id(board);
        if(strcmp(boardObject->valuestring + 3, board + 3)) {
            goto finish;
        }
        SCLOUD_DEBUG("board: %s", boardObject->valuestring);
    }

    cmdObject = aJson.getObjectItem(root, "cmd");
    if(cmdObject == NULL) {
        goto finish;
    } else {
        SCLOUD_DEBUG("cmd: %s", cmdObject->valuestring);
        if(!strcmp("upgradeBin", cmdObject->valuestring)) {
            //在线编程升级
            dtokenObject = aJson.getObjectItem(root, "dwn_token");
            if(dtokenObject == NULL) {
                goto finish;
            }
            intorobot_ota_upgrade(dtokenObject->valuestring, "");
        } else if(!strcmp("upgradeSubsys", cmdObject->valuestring)) {
            //升级子系统
            versionObject = aJson.getObjectItem(root, "version");
            if(versionObject == NULL) {
                goto finish;
            } else {
                char sys_ver[24]={0};
                HAL_PARAMS_Get_System_subsys_ver(sys_ver, sizeof(sys_ver));
                if(!strcmp(sys_ver, versionObject->valuestring)) {
                    goto finish;
                }
            }
            intorobot_subsys_upgrade(versionObject->valuestring);
        } else if(!strcmp("upgradeApp", cmdObject->valuestring)) {
            //应用固件升级
            //intorobot_firmware_upgrade(versionObject->valuestring)
        } else if(!strcmp("reboot", cmdObject->valuestring)) {
            //启动处理
            send_upgrade_status(UPGRADE_REPLY_REBOOT_READY, 0);
            HAL_Core_System_Reset();
        }
    }
finish:
    if (root != NULL)
    {aJson.deleteItem(root);}
}

void cloud_datapoint_receive_callback(uint8_t *payload, uint32_t len)
{
    SCLOUD_DEBUG("Ok! receive datapoint form cloud!");
    intorobotParseReceiveDataJson(payload, len);
}

void cloud_debug_callback(uint8_t *payload, uint32_t len)
{
    SCLOUD_DEBUG("system debug recieve!");
    mqtt_receive_debug_info(payload, len);
}

void fill_mqtt_topic(String &fulltopic, api_version_t version, const char *topic, const char *device_id)
{
    String sdevice_id=intorobot_deviceID();

    if( API_VERSION_V1 == version ) {
        if(device_id == NULL) {
            fulltopic = "v1/" + sdevice_id + "/";
        } else {
            fulltopic = "v1/" + String(device_id) + "/";
        }
    } else {
        if(device_id == NULL) {
            fulltopic = "v2/device/" + sdevice_id + "/";
        } else {
            fulltopic = "v2/device/" + String(device_id) + "/";
        }
    }
    fulltopic+=topic;
}

void intorobot_cloud_init(void)
{
    memset(&g_debug_tx_buffer,0,sizeof(g_debug_tx_buffer));
    memset(&g_debug_rx_buffer,0,sizeof(g_debug_rx_buffer));

    g_mqtt_client = MqttClientClass((char *)INTOROBOT_SERVER_DOMAIN, INTOROBOT_SERVER_PORT, mqtt_client_callback, g_mqtt_tcp_client);
    // v1版本subscibe
#if 1
    intorobot_subscribe(API_VERSION_V1, INTOROBOT_MQTT_SUB_UPDATE_TOPIC, NULL, ota_update_callback, 0);                 //固件升级
    intorobot_subscribe(API_VERSION_V1, INTOROBOT_MQTT_SUB_JSON_UPDATE_TOPIC, NULL, subsys_update_callback, 0);         //子系统升级
#endif
    intorobot_subscribe(API_VERSION_V1, INTOROBOT_MQTT_SUB_REBOOT_TOPIC, NULL, system_reboot_callback, 0);              //stm32重启
    intorobot_subscribe(API_VERSION_V1, INTOROBOT_MQTT_SUB_RECEIVE_DEBUG_TOPIC, NULL, system_debug_callback, 0);        //从平台获取调试信息

    // v2版本subscibe
    intorobot_subscribe(API_VERSION_V2, INTOROBOT_MQTT_TX_TOPIC, NULL, cloud_datapoint_receive_callback, 0); //从平台获取数据通讯信息
#if 0
    intorobot_subscribe(API_VERSION_V2, INTOROBOT_MQTT_ACTION_TOPIC, NULL, cloud_action_callback, 0);        //从平台获取系统控制信息
    intorobot_subscribe(API_VERSION_V2, INTOROBOT_MQTT_DEBUGTX_TOPIC, NULL, cloud_debug_callback, 0);        //从平台获取调试信息
#endif

    // 添加默认数据点
    intorobotDefineDataPointBool(0xFF80, UP_DOWN, false, "", 0);//reboot
    intorobotDefineDataPointBool(0xFF81, UP_DOWN, false, "", 0);//write all datapoint
}

bool intorobot_publish(api_version_t version, const char* topic, uint8_t* payload, unsigned int plength, uint8_t qos, uint8_t retained)
{
    String fulltopic;

    fill_mqtt_topic(fulltopic, version, topic, NULL);
    SYSTEM_THREAD_CONTEXT_SYNC_CALL_RESULT(g_mqtt_client.publish(fulltopic.c_str(), payload, plength, retained));
}

bool intorobot_subscribe(api_version_t version, const char* topic, const char *device_id, void (*callback)(uint8_t*, uint32_t), uint8_t qos)
{
    String fulltopic;

    add_subscribe_callback(version, (char *)topic, (char *)device_id, callback, qos);
    fill_mqtt_topic(fulltopic, version, topic, device_id);

    SYSTEM_THREAD_CONTEXT_SYNC_CALL_RESULT(g_mqtt_client.subscribe(fulltopic.c_str(), qos));
}

bool intorobot_widget_subscribe(api_version_t version, const char* topic, const char *device_id, WidgetBaseClass *pWidgetBase, uint8_t qos)
{
    String fulltopic;

    add_widget_subscibe_callback(version, (char *)topic, (char *)device_id, pWidgetBase, qos);
    fill_mqtt_topic(fulltopic, version, topic, device_id);
    SYSTEM_THREAD_CONTEXT_SYNC_CALL_RESULT(g_mqtt_client.subscribe(fulltopic.c_str(), qos));
}

bool intorobot_unsubscribe(api_version_t version, const char *topic, const char *device_id)
{
    String fulltopic;

    del_subscribe_callback(version, (char *)topic, (char *)device_id);
    fill_mqtt_topic(fulltopic, version, topic, device_id);
    SYSTEM_THREAD_CONTEXT_SYNC_CALL_RESULT(g_mqtt_client.unsubscribe(fulltopic.c_str()));
}

size_t intorobot_debug_info_write(uint8_t byte)
{
    g_debug_tx_buffer.buffer[g_debug_tx_buffer.head] = byte;
    g_debug_tx_buffer.head++;
    /* To avoid buffer overflow */
    if(g_debug_tx_buffer.head == CLOUD_DEBUG_BUFFER_SIZE) {
        g_debug_tx_buffer.head = 0;
    }
    return 1;
}

int intorobot_debug_info_read(void)
{
    // if the head isn't ahead of the tail, we don't have any characters
    if (g_debug_rx_buffer.head == g_debug_rx_buffer.tail) {
        return -1;
    } else {
        unsigned char c = g_debug_rx_buffer.buffer[g_debug_rx_buffer.tail];
        g_debug_rx_buffer.tail = (unsigned int)(g_debug_rx_buffer.tail + 1) % CLOUD_DEBUG_BUFFER_SIZE;
        return c;
    }
}

int intorobot_debug_info_available(void)
{
    return (unsigned int)(CLOUD_DEBUG_BUFFER_SIZE + g_debug_rx_buffer.head - g_debug_rx_buffer.tail) % CLOUD_DEBUG_BUFFER_SIZE;
}


pCallBack get_subscribe_callback(char * fulltopic)
{
    char topictmp[128]={0};
    char device_id[32]={0};

    for (int i = 0 ; i < g_callback_list.total_callbacks; i++)
    {
        memset(topictmp, 0, sizeof(topictmp));
        if( API_VERSION_V1 == g_callback_list.callback_node[i].version ) {
            if(g_callback_list.callback_node[i].device_id == NULL) {
                HAL_PARAMS_Get_System_device_id(device_id, sizeof(device_id));
                sprintf(topictmp,"v1/%s/", device_id);
            } else {
                sprintf(topictmp,"v1/%s/", g_callback_list.callback_node[i].device_id);
            }
        } else {
            if(g_callback_list.callback_node[i].device_id == NULL) {
                HAL_PARAMS_Get_System_device_id(device_id, sizeof(device_id));
                sprintf(topictmp,"v2/device/%s/", device_id);
            } else {
                sprintf(topictmp,"v2/device/%s/", g_callback_list.callback_node[i].device_id);
            }
        }
        strcat(topictmp,g_callback_list.callback_node[i].topic);
        if (strcmp(fulltopic, topictmp) == 0) {
            return g_callback_list.callback_node[i].callback;
        }
    }
    return NULL;
}

void add_subscribe_callback(api_version_t version, char *topic, char *device_id, void (*callback)(uint8_t*, uint32_t), uint8_t qos)
{
    int if_found_topic = 0;
    int i = 0;

    for (i = 0 ; i < g_callback_list.total_callbacks; i++)
    {
        if ((topic == g_callback_list.callback_node[i].topic)
                &&(device_id == g_callback_list.callback_node[i].device_id)
                &&(version == g_callback_list.callback_node[i].version)
                ) {
            if_found_topic = 1;
            break;
        }
    }

    if (if_found_topic) {
        g_callback_list.callback_node[i].callback = callback;
        g_callback_list.callback_node[i].qos = qos;
    } else {
        if (g_callback_list.total_callbacks == MAX_CALLBACK_NUM) {
            return;
        } else {
            g_callback_list.callback_node[g_callback_list.total_callbacks].callback = callback;
            g_callback_list.callback_node[g_callback_list.total_callbacks].qos = qos;
            g_callback_list.callback_node[g_callback_list.total_callbacks].topic = topic;
            g_callback_list.callback_node[g_callback_list.total_callbacks].device_id = device_id;
            g_callback_list.callback_node[g_callback_list.total_callbacks].version = version;
            g_callback_list.total_callbacks ++;
        }
    }
}

WidgetBaseClass *get_widget_subscribe_callback(char * fulltopic)
{
    char topictmp[128]={0};
    char device_id[32]={0};

    for (int i = 0 ; i < g_callback_list.total_wcallbacks; i++)
    {
        memset(topictmp, 0, sizeof(topictmp));
        if( API_VERSION_V1 == g_callback_list.callback_node[i].version ) {
            if(g_callback_list.widget_callback_node[i].device_id == NULL) {
                HAL_PARAMS_Get_System_device_id(device_id, sizeof(device_id));
                sprintf(topictmp,"v1/%s/", device_id);
            } else {
                sprintf(topictmp,"v1/%s/", g_callback_list.widget_callback_node[i].device_id);
            }
        } else {
            if(g_callback_list.widget_callback_node[i].device_id == NULL) {
                HAL_PARAMS_Get_System_device_id(device_id, sizeof(device_id));
                sprintf(topictmp,"v2/device/%s/", device_id);
            } else {
                sprintf(topictmp,"v2/device/%s/", g_callback_list.widget_callback_node[i].device_id);
            }
        }
        strcat(topictmp,g_callback_list.widget_callback_node[i].topic);
        if (strcmp(fulltopic, topictmp) == 0) {
            return g_callback_list.widget_callback_node[i].pWidgetBase;
        }
    }
    return NULL;
}

void add_widget_subscibe_callback(api_version_t version, char *topic, char *device_id, WidgetBaseClass *pWidgetBase, uint8_t qos)
{
    int if_found_topic = 0;
    int i = 0;

    for (i = 0 ; i < g_callback_list.total_wcallbacks; i++)
    {
        if ((topic == g_callback_list.widget_callback_node[i].topic)
                &&(device_id == g_callback_list.widget_callback_node[i].device_id)
                &&(version == g_callback_list.widget_callback_node[i].version)
                ) {
            if_found_topic = 1;
            break;
        }
    }

    if (if_found_topic) {
        g_callback_list.widget_callback_node[i].pWidgetBase = pWidgetBase;
        g_callback_list.widget_callback_node[i].qos = qos;
    } else {
        if (g_callback_list.total_wcallbacks == MAX_CALLBACK_NUM) {
            return;
        } else {
            g_callback_list.widget_callback_node[g_callback_list.total_wcallbacks].pWidgetBase = pWidgetBase;
            g_callback_list.widget_callback_node[g_callback_list.total_wcallbacks].qos = qos;
            g_callback_list.widget_callback_node[g_callback_list.total_wcallbacks].topic = topic;
            g_callback_list.widget_callback_node[g_callback_list.total_wcallbacks].device_id = device_id;
            g_callback_list.widget_callback_node[g_callback_list.total_wcallbacks].version = version;
            g_callback_list.total_wcallbacks ++;
        }
    }
}

void del_subscribe_callback(api_version_t version, char * topic, char *device_id)
{
    for (int i = 0 ; i < g_callback_list.total_callbacks; i++)
    {
        if ((topic == g_callback_list.callback_node[i].topic)
                && (device_id == g_callback_list.callback_node[i].device_id)
                && (version == g_callback_list.callback_node[i].version)
                ) {
            memcpy(&g_callback_list.callback_node[i], &g_callback_list.callback_node[i+1], (g_callback_list.total_callbacks - 1 - i) * sizeof(struct CallBackNode));
            memset(&g_callback_list.callback_node[g_callback_list.total_callbacks-1], 0, sizeof(struct CallBackNode));
            g_callback_list.total_callbacks--;
            return;
        }
    }

    for (int i = 0 ; i < g_callback_list.total_wcallbacks; i++)
    {
        if ((topic == g_callback_list.widget_callback_node[i].topic)
                && (device_id == g_callback_list.widget_callback_node[i].device_id)
                && (version == g_callback_list.widget_callback_node[i].version)
                ) {
            memcpy(&g_callback_list.widget_callback_node[i], &g_callback_list.widget_callback_node[i+1], (g_callback_list.total_wcallbacks - 1 - i) * sizeof(struct WidgetCallBackNode));
            memset(&g_callback_list.widget_callback_node[g_callback_list.total_wcallbacks-1], 0, sizeof(struct WidgetCallBackNode));
            g_callback_list.total_wcallbacks--;
            return;
        }
    }
}

void resubscribe(void)
{
    for (int i = 0 ; i < g_callback_list.total_callbacks; i++)
    {
        intorobot_subscribe(g_callback_list.callback_node[i].version, g_callback_list.callback_node[i].topic, g_callback_list.callback_node[i].device_id,
                            g_callback_list.callback_node[i].callback, g_callback_list.callback_node[i].qos);
    }

    for (int i = 0 ; i < g_callback_list.total_wcallbacks; i++)
    {
        intorobot_widget_subscribe(g_callback_list.widget_callback_node[i].version, g_callback_list.widget_callback_node[i].topic, g_callback_list.widget_callback_node[i].device_id,
                            g_callback_list.widget_callback_node[i].pWidgetBase, g_callback_list.widget_callback_node[i].qos);
    }
}

void mqtt_send_debug_info(void)
{
    uint32_t n;
    String s_debug_info="";

    for(n=0; (n<MQTT_MAX_PACKET_SIZE)&&(g_debug_tx_buffer.tail!=g_debug_tx_buffer.head); n++)
    {
        s_debug_info += (char)g_debug_tx_buffer.buffer[g_debug_tx_buffer.tail];
        g_debug_tx_buffer.tail = (unsigned int)(g_debug_tx_buffer.tail + 1) % CLOUD_DEBUG_BUFFER_SIZE;
    }

    if(n) {
        intorobot_publish(API_VERSION_V1, INTOROBOT_MQTT_SEND_DEBUG_TOPIC, (uint8_t *)s_debug_info.c_str(), n, 0, false);
        intorobot_publish(API_VERSION_V2, INTOROBOT_MQTT_DEBUGRX_TOPIC, (uint8_t *)s_debug_info.c_str(), n, 0, false);
    }
}

void mqtt_receive_debug_info(uint8_t *pIn, uint32_t len)
{
    int n;

    for( n=0; n<len; n++)
    {
        g_debug_rx_buffer.buffer[g_debug_rx_buffer.head] = pIn[n];
        g_debug_rx_buffer.head++;
        /* To avoid buffer overflow */
        if(g_debug_rx_buffer.head == CLOUD_DEBUG_BUFFER_SIZE) {
            g_debug_rx_buffer.head = 0;
        }
    }
}

/**
 * Determines if the system will attempt to connect or disconnect from the cloud.
 */
bool intorobot_cloud_flag_connected(void)
{
    if (g_intorobot_network_connected && g_intorobot_cloud_connected) {
        return true;
    } else {
        return false;
    }
}

void intorobot_cloud_disconnect(void)
{
    if(g_mqtt_client.connected()) {
        g_mqtt_client.disconnect();
    }
}

int intorobot_cloud_connect(void)
{
    SCLOUD_DEBUG("---------mqtt connect start--------");
    intorobot_cloud_disconnect();
    //mqtt server domain
    char sv_domain[32]={0};
    HAL_PARAMS_Get_System_sv_domain(sv_domain, sizeof(sv_domain));
    if(0 == strlen(sv_domain)) {
        strcpy(sv_domain, INTOROBOT_SERVER_DOMAIN);
    }
    //mqtt server port
    int sv_port=HAL_PARAMS_Get_System_sv_port();
    if(sv_port <= 0) {
        sv_port=INTOROBOT_SERVER_PORT;
    }
    //strcpy(sv_domain, "112.124.117.64");
    //sv_port=1885;
    g_mqtt_client.setServer(sv_domain, sv_port);

    char device_id[38]={0}, activation_code[38]={0}, access_token[38]={0}, dw_domain[38]={0};
    HAL_PARAMS_Get_System_device_id(device_id, sizeof(device_id));
    HAL_PARAMS_Get_System_access_token(access_token, sizeof(access_token));
    HAL_PARAMS_Get_System_activation_code(activation_code, sizeof(activation_code));
    HAL_PARAMS_Get_System_dw_domain(dw_domain, sizeof(dw_domain));

    SCLOUD_DEBUG("---------terminal params--------");
    SCLOUD_DEBUG("mqtt domain     : %s", sv_domain);
    SCLOUD_DEBUG("mqtt port       : %d", sv_port);
    SCLOUD_DEBUG("down domain     : %s", dw_domain);
    SCLOUD_DEBUG("at_mode         : %d", HAL_PARAMS_Get_System_at_mode());
    SCLOUD_DEBUG("zone            : %f", HAL_PARAMS_Get_System_zone());
    SCLOUD_DEBUG("device_id       : %s", device_id);
    SCLOUD_DEBUG("activation_code : %s", activation_code);
    SCLOUD_DEBUG("access_token    : %s", access_token);
    SCLOUD_DEBUG("--------------------------------");

    String fulltopic;
    fill_mqtt_topic(fulltopic, API_VERSION_V2, INTOROBOT_MQTT_WILL_TOPIC, NULL);
    //client id change to device id. chenkaiyao 2016-01-17
    if(g_mqtt_client.connect(device_id, access_token, device_id, fulltopic, 0, true, INTOROBOT_MQTT_WILL_MESSAGE)) {
        SCLOUD_DEBUG("---------connect success--------");
        char fw_version[28]="", subsys_version[28]="", board[32]="";
        product_details_t product_details;

        HAL_PARAMS_Get_System_fwlib_ver(fw_version, sizeof(fw_version));
        HAL_PARAMS_Get_System_subsys_ver(subsys_version, sizeof(subsys_version));
        system_platform_id(board);
        system_product_instance().get_product_details(product_details);

        aJsonClass aJson;
        //intorobot 平台上送数据
        aJsonObject* root = aJson.createObject();
        if (root == NULL)
        {return -1;}
        aJson.addStringToObject(root, "fw_ver", fw_version);
        aJson.addStringToObject(root, "sys_ver", subsys_version);
        char* string = aJson.print(root);
        intorobot_publish(API_VERSION_V1, INTOROBOT_MQTT_VERSION_TOPIC, (uint8_t*)string, strlen(string), 0, true);
        free(string);
        aJson.deleteItem(root);

        //intoYun   平台上送数据
        root = aJson.createObject();
        if (root == NULL) {
            return -1;
        }
        aJson.addStringToObject(root, "productId", product_details.product_id);
        if(PRODUCT_MODE_MASTER == product_details.product_mode)
        {aJson.addStringToObject(root, "productMode", "master");}
        else
        {aJson.addStringToObject(root, "productMode", "slave");}
        aJson.addStringToObject(root, "board", board);
        aJson.addStringToObject(root, "productVer", String(product_details.product_firmware_version).c_str());
        aJson.addStringToObject(root, "libVer", fw_version);
        aJson.addStringToObject(root, "subsysVer", subsys_version);
        aJson.addBooleanToObject(root, "online", true);
        string = aJson.print(root);
        intorobot_publish(API_VERSION_V2, INTOROBOT_MQTT_WILL_TOPIC, (uint8_t*)string, strlen(string), 0, true);
        free(string);
        aJson.deleteItem(root);
        //重新订阅
        SCLOUD_DEBUG("---------mqtt resubscribe--------");
        resubscribe();
        return 0;
    }
    SCLOUD_DEBUG("---------connect failed--------");
    return -1;
}

int intorobot_cloud_handle(void)
{
    bool reboot_flag = false, all_datapoint_flag = false;
    if(true == g_mqtt_client.loop()) {
        //reboot
        if(RESULT_DATAPOINT_NEW == intorobotReadDataPointBool(0xFF80, reboot_flag)) {
            intorobot_cloud_disconnect();
            delay(500);
            HAL_Core_System_Reset();
        }
        //write all datepoint
        if(RESULT_DATAPOINT_NEW == intorobotReadDataPointBool(0xFF81, all_datapoint_flag)) {
            intorobotSendDataPointAll();
        }
        //发送IntoRobot.printf打印到平台
        mqtt_send_debug_info();
        return 0;
    }
    return -1;
}

static UDP ntp_time_udp;
/*Send the request packet to the NTP server.*/
static void send_ntp_request_packet(IPAddress timeServerIP)
{
    uint8_t packetBuffer[48];

    memset(packetBuffer, 0, sizeof(packetBuffer));
    packetBuffer[0] = 0b11100011; // LI, Version, Mode
    packetBuffer[1] = 0;          // Stratum, or type of clock
    packetBuffer[2] = 6;          // Polling Interval
    packetBuffer[3] = 0xEC;       // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    packetBuffer[12]  = 49;
    packetBuffer[13]  = 0x4E;
    packetBuffer[14]  = 49;
    packetBuffer[15]  = 52;
    ntp_time_udp.beginPacket(timeServerIP, (int)123); // NTP Server and Port
    ntp_time_udp.write(packetBuffer, 48);
    ntp_time_udp.endPacket();
}

static time_t get_ntp_time(void)
{
    uint8_t packetBuffer[48];
#ifdef configWIRING_WIFI_ENABLE
    IPAddress ntpServer = WiFi.resolve(NTP_TIMESERVER);
#else
    IPAddress ntpServer = Cellular.resolve(NTP_TIMESERVER);
#endif

    ntp_time_udp.begin(8888);
    for (int i = 0 ; i < 4 ; i++)
    {
        send_ntp_request_packet(ntpServer);
        uint32_t beginWait = millis();
        while (millis() - beginWait < 2000)
        {
            if (ntp_time_udp.parsePacket()) {
                ntp_time_udp.read(packetBuffer, 48);
                unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
                unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
                unsigned long secSince1900 = highWord << 16 | lowWord;
                ntp_time_udp.stop();
                return secSince1900 - 2208988800UL;
            }
        }
    }
    ntp_time_udp.stop();
    return 0;
}

bool intorobot_sync_time(void)
{
    SCLOUD_DEBUG("---------device syncTime begin---------");
    time_t utc_time = get_ntp_time();
    if(utc_time) {
        SCLOUD_DEBUG("device syncTime success! utc_time = %d",utc_time);
        Time.setTime(utc_time);
        return true;
    }
    SCLOUD_DEBUG("device syncTime failed!");
    return false;
}

bool intorobot_device_register(void)
{
    SCLOUD_DEBUG("---------device register begin---------");

    HTTPClient http;
    aJsonClass aJson;
    bool flag = false;

    aJsonObject* root = aJson.createObject();
    if (root == NULL)
    {return false;}

    //获取product id
    product_details_t product_details;
    system_product_instance().get_product_details(product_details);
    aJson.addStringToObject(root, "productId", product_details.product_id);

    //获取utc 时间戳
    time_t utc_time = Time.now();
    aJson.addStringToObject(root, "timestamp", String(utc_time).c_str());

    //计算签名 signature = md5(timestamp + productSecret)
    MD5Builder md5;
    String payload = "";

    payload += utc_time;
    payload += product_details.product_secret;
    md5.begin();
    md5.add((uint8_t *)payload.c_str(), payload.length());
    md5.calculate();
    aJson.addStringToObject(root, "signature", md5.toString().c_str());
    char* string = aJson.print(root);
    payload = string;
    free(string);
    aJson.deleteItem(root);

    http.begin(INTOROBOT_HTTP_DOMAIN, INTOROBOT_HTTP_PORT, "/v1/device?act=register");
    http.setUserAgent(F("User-Agent: Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.101 Safari/537.36"));
    int httpCode = http.POST(payload);
    if(httpCode == HTTP_CODE_OK) {
        payload = http.getString();

        root = aJson.parse((char *)payload.c_str());
        if (root == NULL)
        {return false;}

        //device_id  and activation_code
        aJsonObject *deviceIdObject = aJson.getObjectItem(root, "deviceId");
        aJsonObject *activationCodeObject = aJson.getObjectItem(root, "activationCode");
        if ( deviceIdObject != NULL && activationCodeObject != NULL) {
            HAL_PARAMS_Set_System_device_id(deviceIdObject->valuestring);
            HAL_PARAMS_Set_System_activation_code(activationCodeObject->valuestring);
            HAL_PARAMS_Set_System_at_mode(AT_MODE_FLAG_OTAA_INACTIVE);
            HAL_PARAMS_Save_Params();
            SCLOUD_DEBUG("device_id       : %s", deviceIdObject->valuestring);
            SCLOUD_DEBUG("activation_code : %s", activationCodeObject->valuestring);
            SCLOUD_DEBUG("device register success!");
            flag = true;
        }
        aJson.deleteItem(root);
    }

    SCLOUD_DEBUG("device register failed!");
    http.end();
    if(flag) {
        return true;
    } else {
        return false;
    }
}

bool intorobot_device_activate(void)
{
    SCLOUD_DEBUG("---------device activate begin---------");

    HTTPClient http;
    aJsonClass aJson;
    bool flag = false;

    aJsonObject* root = aJson.createObject();
    if (root == NULL)
    {return false;}

    //获取product id
    char device_id[38]={0}, activation_code[38]={0};
    HAL_PARAMS_Get_System_device_id(device_id, sizeof(device_id));
    HAL_PARAMS_Get_System_activation_code(activation_code, sizeof(activation_code));
    aJson.addStringToObject(root, "deviceId", device_id);

    //获取utc 时间戳
    time_t utc_time = Time.now();
    aJson.addStringToObject(root, "timestamp", String(utc_time).c_str());

    //计算签名 signature = md5(timestamp + productSecret)
    MD5Builder md5;
    String payload = "";

    payload += utc_time;
    payload += activation_code;
    md5.begin();
    md5.add((uint8_t *)payload.c_str(), payload.length());
    md5.calculate();
    aJson.addStringToObject(root, "signature", md5.toString().c_str());
    char* string = aJson.print(root);
    payload = string;
    free(string);
    aJson.deleteItem(root);

    http.begin(INTOROBOT_HTTP_DOMAIN, INTOROBOT_HTTP_PORT, "/v1/device?act=activate");
    http.setUserAgent(F("User-Agent: Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.101 Safari/537.36"));
    int httpCode = http.POST(payload);
    if(httpCode == HTTP_CODE_OK) {
        payload = http.getString();

        root = aJson.parse((char *)payload.c_str());
        if (root == NULL)
        {return false;}

        //device_id  and activation_code
        aJsonObject *accessTokenObject = aJson.getObjectItem(root, "token");
        if ( accessTokenObject != NULL ) {
            HAL_PARAMS_Set_System_access_token(accessTokenObject->valuestring);
            HAL_PARAMS_Set_System_at_mode(AT_MODE_FLAG_OTAA_ACTIVE);
            HAL_PARAMS_Save_Params();
            SCLOUD_DEBUG("access_token  : %s", accessTokenObject->valuestring);
            SCLOUD_DEBUG("device activate success!");
            flag = true;
        }
        aJson.deleteItem(root);
    }

    SCLOUD_DEBUG("device activate failed!");
    http.end();
    if(flag) {
        return true;
    } else {
        return false;
    }
}

bool intorobot_get_version(String &body)
{
    return true;
}

#endif

String intorobot_deviceID(void)
{
    char device_id[32]={0};

    HAL_PARAMS_Get_System_device_id(device_id, sizeof(device_id));
    return device_id;
}

volatile uint8_t intorobot_process_flag = 0;
void intorobot_process(void)
{
    intorobot_process_flag = 1;

    // application thread will pump application messages
#if PLATFORM_THREADING
    if (system_thread_get_state(NULL) && APPLICATION_THREAD_CURRENT()) {
        ApplicationThread.process();
        return;
    }
#endif

    // run the background processing loop, and specifically also pump cloud events
    system_process_loop();
    intorobot_process_flag = 0;
}

