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

#include "firmware_platform_config.h"

#include <stdlib.h>
#include <stdio.h>

#include "hw_config.h"
#include "core_hal.h"
#include "params_hal.h"
#include "updater_hal.h"
#include "wiring_string.h"
#include "wiring_tcpclient.h"
#include "wiring_udp.h"
#include "wiring_wifi.h"
#include "wiring_cellular.h"
#include "wiring_system.h"
#include "wiring_ticks.h"
#include "wiring_random.h"
#include "wiring_httpclient.h"
#include "wiring_time.h"
#include "system_cloud_def.h"
#include "system_datapoint.h"
#include "system_cloud.h"
#include "system_mode.h"
#include "system_task.h"
#include "system_threading.h"
#include "system_update.h"
#include "system_rgbled.h"
#include "system_product.h"
#include "system_update.h"
#include "system_utilities.h"
#include "system_config.h"
#include "string_convert.h"
#include "wiring_ajson.h"
#include "wiring_httpdownload.h"
#include "intorobot_def.h"

const static char *TAG = "sys-cloud";

using namespace intorobot;

#if FIRMWARE_CONFIG_SYSTEM_CLOUD
#include "wiring_mqttclient.h"
#include "mqttcrypto.h"

volatile uint8_t INTOROBOT_CLOUD_CONNECT_PREPARED = 0;   //平台链接预处理状态 1已经处理
volatile uint8_t INTOROBOT_CLOUD_CONNECTED        = 0;   //平台连接状态 1连接上了

uint8_t g_mqtt_appskey[16] = {0};
uint8_t g_mqtt_nwkskey[16] = {0};
uint16_t g_up_seq_id = 0;
uint8_t g_download_progress = 0;

struct CallBackList g_callback_list;  //回调结构体
struct CloudDebugBuffer  g_debug_tx_buffer;
struct CloudDebugBuffer  g_debug_rx_buffer;

uint16_t g_keepAlive = 0;

TCPClient g_mqtt_tcp_client;
MqttClientClass g_mqtt_client;
RGBLEDState led_state;


static pCallBack get_subscribe_callback(char * fulltopic);
static WidgetBaseClass *get_widget_subscribe_callback(char * fulltopic);
static void add_subscribe_callback(topic_version_t version, char *topic, char *device_id, void (*callback)(uint8_t*, uint32_t), uint8_t qos);
static void add_widget_subscibe_callback(topic_version_t version, char *topic, char *device_id, WidgetBaseClass *pWidgetBase, uint8_t qos);
static void del_subscribe_callback(topic_version_t version, char * topic, char *device_id);
static void mqtt_receive_debug_info(uint8_t *pIn, uint32_t len);

void mqtt_client_callback(char *topic, uint8_t *payload, uint32_t length)
{
    uint8_t *pdata = NULL;
    uint16_t datalen = length-6; //去掉seq_id + mic
    char device_id[38] = {0};
    uint8_t mic[4];
    uint16_t down_seq_id;

    MOLMC_LOGD(TAG, "mqtt callback!");
    MOLMC_LOGD(TAG, "topic: %s", topic);

    //MOLMC_LOGD(TAG, "mqtt receive data:");
    //MOLMC_LOGD_BUFFER_HEX(TAG, payload, length);

    pdata = (uint8_t *)malloc(datalen+1);
    if(NULL == pdata) {
        return;
    }
    memset(pdata, 0, datalen+1);

    HAL_PARAMS_Get_System_device_id(device_id, sizeof(device_id));
    MqttComputeMic( payload, length-4, g_mqtt_nwkskey, mic );
    if(!memcmp(&payload[length-4], mic, 4)) {
        down_seq_id = (payload[0] << 8) + payload[1];
        if(System.featureEnabled(SYSTEM_FEATURE_CLOUD_DATA_ENCRYPT_ENABLED)) {
            MqttPayloadDecrypt( &payload[2], datalen, g_mqtt_appskey, 1, down_seq_id, device_id, pdata );
        } else {
            memcpy(pdata, &payload[2], datalen);
        }
    }

    pCallBack pcallback=get_subscribe_callback(topic);
    if(pcallback!=NULL) {
        pcallback(pdata, datalen);
    }

    WidgetBaseClass *pwidgetbase=get_widget_subscribe_callback(topic);
    if(pwidgetbase!=NULL) {
        pwidgetbase->widgetBaseCallBack(pdata, datalen);
    }

    free(pdata);
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
    intorobot_publish(TOPIC_VERSION_V2, INTOROBOT_MQTT_REPLY_TOPIC, (uint8_t*)status.c_str(), status.length(), 0, false);
}

static void intorobot_app_upgrade_progress(uint8_t *data, size_t len, uint32_t currentSize, uint32_t totalSize)
{
    uint8_t progress = (currentSize * 100)/totalSize;

    //MOLMC_LOGD(TAG, "progress len = %d, currentSize = %d, totalSize = %d, g_download_progress = %d", len, currentSize, totalSize, g_download_progress);
    if((progress > 0) && (progress - g_download_progress) > 30) {
        g_download_progress = progress;
        send_upgrade_status(UPGRADE_REPLY_PROGRESS, progress);
    }
}

//在线编程升级
static void intorobot_program_upgrade(const char *token, const char *md5)
{
    MOLMC_LOGD(TAG, "v2 :online program upgrade!");

    bool flag = false;
    String domain = "", param = "";

    led_state.save();
    system_rgb_color(RGB_COLOR_YELLOW);

    send_upgrade_status(UPGRADE_REPLY_READY, 0);

    char down_domain[36]={0};
    HAL_PARAMS_Get_System_dw_domain(down_domain, sizeof(down_domain));
    domain+=down_domain;
    param += String(INTOROBOT_OTA_UPDATE_URL) + "?dwn_token=" + String(token);

    HTTPDownload httpUpdate;
    String url="http://" + domain + param;
    MOLMC_LOGD(TAG, "url = %s!", url.c_str());
    httpUpdate.onProgress(intorobot_app_upgrade_progress);
    http_download_return_t ret = httpUpdate.update(url);
    switch(ret) {
        case HTTP_DOWNLOAD_OK:
            MOLMC_LOGD(TAG, "v2 :HTTP_DOWNLOAD_OK!");
            flag = true;
            break;
        default:
            MOLMC_LOGD(TAG, "v2 :HTTP_UPDATE_FAIL!");
            break;
    }

    if(flag) {
        send_upgrade_status(UPGRADE_REPLY_DOWN_SUCC_EXIT, 0);
        delay(500);
        HAL_Core_System_Reset();
        while(1); //不会运行到地方
    } else {
        send_upgrade_status(UPGRADE_REPLY_DOWN_FAIL, 0);
    }
    led_state.restore();
}

//固件升级
static void intorobot_ota_upgrade(bool force, uint32_t size, const char *md5, const char *ver, const char *url)
{
    MOLMC_LOGD(TAG, "v2 :ota upgrade!");

    bool flag = false;

    led_state.save();
    system_rgb_color(RGB_COLOR_YELLOW);

    send_upgrade_status(UPGRADE_REPLY_READY, 0);

    HTTPDownload httpUpdate;
    httpUpdate.onProgress(intorobot_app_upgrade_progress);
    http_download_return_t ret = httpUpdate.update(url);
    switch(ret) {
        case HTTP_DOWNLOAD_OK:
            MOLMC_LOGD(TAG, "v2 :HTTP_DOWNLOAD_OK!");
            flag = true;
            break;
        default:
            MOLMC_LOGD(TAG, "v2 :HTTP_UPDATE_FAIL!");
            break;
    }

    if(flag) {
        send_upgrade_status(UPGRADE_REPLY_DOWN_SUCC_EXIT, 0);
        delay(500);
        HAL_Core_System_Reset();
        while(1); //不会运行到地方
    } else {
        send_upgrade_status(UPGRADE_REPLY_DOWN_FAIL, 0);
    }
    led_state.restore();
}

void cloud_action_callback(uint8_t *payload, uint32_t len)
{
    MOLMC_LOGD(TAG, "v2 :cloud_action_callback!");
    MOLMC_LOGD(TAG, "payload: %s", payload);

    aJsonClass aJson;
    String s_payload;
    aJsonObject *root = NULL, *boardObject = NULL, *cmdObject = NULL, *dtokenObject = NULL, *versionObject = NULL;

    for(int n=0; n<len; n++) {
        s_payload+=(char)payload[n];
    }

    g_download_progress = 0;

    root = aJson.parse((char *)s_payload.c_str());
    if(root == NULL) {
        goto failure;
    }

    cmdObject = aJson.getObjectItem(root, "cmd");
    if((cmdObject == NULL) || (cmdObject->type != aJson_String)) {
        goto failure;
    } else {
        MOLMC_LOGD(TAG, "cmd: %s", cmdObject->valuestring);
        if(!strcmp("upgradeBin", cmdObject->valuestring)) { //在线编程升级
            boardObject = aJson.getObjectItem(root, "board");
            if((boardObject == NULL) || (boardObject->type != aJson_String)) {
                goto failure;
            } else {
                char board[32]="";
                system_get_board_id(board, sizeof(board));
                if(strcmp(boardObject->valuestring + 3, board + 3)) {
                    goto failure;
                }
                MOLMC_LOGD(TAG, "board: %s", boardObject->valuestring);
            }

            dtokenObject = aJson.getObjectItem(root, "dwn_token");
            if((dtokenObject == NULL) || (dtokenObject->type != aJson_String)) {
                goto failure;
            }
            intorobot_program_upgrade(dtokenObject->valuestring, "");
            goto finish;
        } else if(!strcmp("upgradeApp", cmdObject->valuestring)) { //应用固件升级
            //取消固件升级内部处理
            if(!System.featureEnabled(SYSTEM_FEATURE_OTA_UPDATE_ENABLED)) {
                system_notify_event(event_cloud_comm, ep_cloud_comm_ota, payload, len);
                goto finish;
            }

            //处理type
            aJsonObject *typeObject = aJson.getObjectItem(root, "type");
            if((typeObject == NULL) || (typeObject->type != aJson_String)) {
                goto failure;
            }
            if(strcmp(typeObject->valuestring, "0")) {
                goto failure;
            }

            //处理force
            bool force = false;
            aJsonObject *forceObject = aJson.getObjectItem(root, "force");
            if((forceObject != NULL) && (forceObject->type == aJson_Boolean)) {
                force = forceObject->valuebool;
            } else {
                goto failure;
            }

            //处理size
            uint32_t size = 0;
            aJsonObject *sizeObject = aJson.getObjectItem(root, "size");
            if((sizeObject != NULL) && (sizeObject->type == aJson_Int)) {
                size = sizeObject->valueint;
            } else {
                goto failure;
            }

            //处理swVer
            char *swVer = NULL;
            aJsonObject *swVerObject = aJson.getObjectItem(root, "swVer");
            if((swVerObject == NULL) || (swVerObject->type != aJson_String)) {
                goto failure;
            } else {
                swVer = swVerObject->valuestring;
            }

            //处理url
            char *url = NULL;
            aJsonObject *urlObject = aJson.getObjectItem(root, "url");
            if((urlObject == NULL) || (urlObject->type != aJson_String)) {
                goto failure;
            } else {
                url = urlObject->valuestring;
            }

            //处理md5
            char *md5 = NULL;
            aJsonObject *md5Object = aJson.getObjectItem(root, "md5");
            if((md5Object == NULL) || (md5Object->type != aJson_String)) {
                goto failure;
            } else {
                md5 = md5Object->valuestring;
            }

            MOLMC_LOGD(TAG, "force: %d", force);
            MOLMC_LOGD(TAG, "size: %d", size);
            MOLMC_LOGD(TAG, "md5: %s", md5);
            MOLMC_LOGD(TAG, "swVer: %s", swVer);
            MOLMC_LOGD(TAG, "url: %s", url);

            intorobot_ota_upgrade(force, size, md5, swVer, url);
            goto finish;
        } else if(!strcmp("reboot", cmdObject->valuestring)) { //启动处理
            send_upgrade_status(UPGRADE_REPLY_REBOOT_READY, 0);
            HAL_Core_System_Reset();
        }
    }

success:
    send_upgrade_status(UPGRADE_REPLY_DOWN_SUCC_EXIT, 0);
    goto finish;

failure:
    send_upgrade_status(UPGRADE_REPLY_DOWN_FAIL, 0);

finish:
    if (root != NULL)
    {aJson.deleteItem(root);}
}

void cloud_data_receive_callback(uint8_t *payload, uint32_t len)
{
    MOLMC_LOGD(TAG, "Ok! receive data form cloud!");

    MOLMC_LOGD(TAG, "OK! Rev datapoint data <%d>: ", len);
    MOLMC_LOGD_BUFFER_HEX(TAG, payload, len);

    if(!System.featureEnabled(SYSTEM_FEATURE_DATA_PROTOCOL_ENABLED)
            || intorobotParseReceiveDatapoints(payload, len)) {
        system_notify_event(event_cloud_comm, ep_cloud_comm_data, payload, len);
    }
}

void cloud_debug_callback(uint8_t *payload, uint32_t len)
{
    MOLMC_LOGD(TAG, "system debug recieve!");
    mqtt_receive_debug_info(payload, len);
}

void fill_mqtt_topic(String &fulltopic, topic_version_t version, const char *topic, const char *device_id)
{
    String sdevice_id = intorobot_deviceID();

    if( TOPIC_VERSION_V1 == version ) {
        if(device_id == NULL) {
            fulltopic = "v1/" + sdevice_id + "/";
        } else {
            fulltopic = "v1/" + String(device_id) + "/";
        }
    } else if(TOPIC_VERSION_V2 == version) {
        if(device_id == NULL) {
            if(PRODUCT_TYPE_GATEWAY == system_get_product_type()) {
                fulltopic = "v2/gateway/" + sdevice_id + "/";
            } else {
                fulltopic = "v2/device/" + sdevice_id + "/";
            }
        } else {
            if(PRODUCT_TYPE_GATEWAY == system_get_product_type()) {
                fulltopic = "v2/gateway/" + String(device_id) + "/";
            } else {
                fulltopic = "v2/device/" + String(device_id) + "/";
            }
        }
    }
    fulltopic+=topic;
}

static bool _intorobot_publish(topic_version_t version, const char* topic, uint8_t* payload, unsigned int plength, uint8_t qos, uint8_t retained)
{
    String fulltopic = "";
    char device_id[38] = {0};
    uint8_t *pdata = (uint8_t *)malloc(plength + 16);
    uint16_t dataIndex = 0;

    if(NULL == pdata) {
        return false;
    }

    g_up_seq_id++;
    pdata[dataIndex++] = ( g_up_seq_id >> 8 ) & 0xFF;
    pdata[dataIndex++] = ( g_up_seq_id ) & 0xFF;

    HAL_PARAMS_Get_System_device_id(device_id, sizeof(device_id));
    if(System.featureEnabled(SYSTEM_FEATURE_CLOUD_DATA_ENCRYPT_ENABLED)) {
        MqttPayloadEncrypt( payload, plength, g_mqtt_appskey, 0, g_up_seq_id, device_id, &pdata[dataIndex] );
    } else {
        memcpy(&pdata[dataIndex], payload, plength);
    }
    dataIndex += plength;
    MqttComputeMic( pdata, dataIndex, g_mqtt_nwkskey, &pdata[dataIndex] );
    dataIndex += 4;

    fill_mqtt_topic(fulltopic, version, topic, NULL);
    //MOLMC_LOGD(TAG, "publish! topic = %s, payload(%d) = %s", fulltopic.c_str(), plength, payload);
    bool result = g_mqtt_client.publish(fulltopic.c_str(), pdata, dataIndex, retained);
    free(pdata);
    return result;
}

bool intorobot_publish(topic_version_t version, const char* topic, uint8_t* payload, unsigned int plength, uint8_t qos, uint8_t retained)
{
    SYSTEM_THREAD_CONTEXT_SYNC_CALL_RESULT(_intorobot_publish(version, topic, payload, plength, qos, retained));
}

bool intorobot_subscribe(topic_version_t version, const char* topic, const char *device_id, void (*callback)(uint8_t*, uint32_t), uint8_t qos)
{
    String fulltopic = "";

    add_subscribe_callback(version, (char *)topic, (char *)device_id, callback, qos);
    fill_mqtt_topic(fulltopic, version, topic, device_id);

    MOLMC_LOGD(TAG, "subscribe! topic = %s", fulltopic.c_str());
    SYSTEM_THREAD_CONTEXT_SYNC_CALL_RESULT(g_mqtt_client.subscribe(fulltopic.c_str(), qos));
}

bool intorobot_widget_subscribe(topic_version_t version, const char* topic, const char *device_id, WidgetBaseClass *pWidgetBase, uint8_t qos)
{
    String fulltopic = "";

    add_widget_subscibe_callback(version, (char *)topic, (char *)device_id, pWidgetBase, qos);
    fill_mqtt_topic(fulltopic, version, topic, device_id);
    SYSTEM_THREAD_CONTEXT_SYNC_CALL_RESULT(g_mqtt_client.subscribe(fulltopic.c_str(), qos));
}

bool intorobot_unsubscribe(topic_version_t version, const char *topic, const char *device_id)
{
    String fulltopic = "";

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


static pCallBack get_subscribe_callback(char * fulltopic)
{
    char topictmp[128] = {0};
    char device_id[32] = {0};

    for (int i = 0 ; i < g_callback_list.total_callbacks; i++) {
        memset(topictmp, 0, sizeof(topictmp));
        if( TOPIC_VERSION_V1 == g_callback_list.callback_node[i].version ) {
            if(g_callback_list.callback_node[i].device_id == NULL) {
                HAL_PARAMS_Get_System_device_id(device_id, sizeof(device_id));
                sprintf(topictmp,"v1/%s/", device_id);
            } else {
                sprintf(topictmp,"v1/%s/", g_callback_list.callback_node[i].device_id);
            }
        } else if( TOPIC_VERSION_V2 == g_callback_list.callback_node[i].version ) {
            if(g_callback_list.callback_node[i].device_id == NULL) {
                HAL_PARAMS_Get_System_device_id(device_id, sizeof(device_id));
                if(PRODUCT_TYPE_GATEWAY == system_get_product_type()) {
                    sprintf(topictmp,"v2/gateway/%s/", device_id);
                } else {
                    sprintf(topictmp,"v2/device/%s/", device_id);
                }
            } else {
                if(PRODUCT_TYPE_GATEWAY == system_get_product_type()) {
                    sprintf(topictmp,"v2/gateway/%s/", g_callback_list.callback_node[i].device_id);
                } else {
                    sprintf(topictmp,"v2/device/%s/", g_callback_list.callback_node[i].device_id);
                }
            }
        }
        strcat(topictmp, g_callback_list.callback_node[i].topic);
        if (strcmp(fulltopic, topictmp) == 0) {
            return g_callback_list.callback_node[i].callback;
        }
    }
    return NULL;
}

static void add_subscribe_callback(topic_version_t version, char *topic, char *device_id, void (*callback)(uint8_t*, uint32_t), uint8_t qos)
{
    int if_found_topic = 0;
    int i = 0;

    if((NULL == topic) || (NULL == callback)) {
        return;
    }

    for (i = 0 ; i < g_callback_list.total_callbacks; i++) {
        if (!strcmp(topic, g_callback_list.callback_node[i].topic) && (version == g_callback_list.callback_node[i].version)) {
            if(((NULL == device_id) && (NULL == g_callback_list.callback_node[i].device_id))
                    || ((NULL != device_id) && (NULL != g_callback_list.callback_node[i].device_id) && !strcmp(device_id, g_callback_list.callback_node[i].device_id))) {
                if_found_topic = 1;
                break;
            }
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
            g_callback_list.callback_node[g_callback_list.total_callbacks].topic = (char *)malloc(strlen(topic)+1);
            if(NULL != g_callback_list.callback_node[g_callback_list.total_callbacks].topic) {
                memset(g_callback_list.callback_node[g_callback_list.total_callbacks].topic, 0, strlen(topic)+1);
                memcpy(g_callback_list.callback_node[g_callback_list.total_callbacks].topic, topic, strlen(topic));
            }
            if(NULL != device_id) {
                g_callback_list.callback_node[g_callback_list.total_callbacks].device_id = (char *)malloc(strlen(device_id)+1);
                if(NULL !=g_callback_list.callback_node[g_callback_list.total_callbacks].device_id) {
                    memset(g_callback_list.callback_node[g_callback_list.total_callbacks].device_id, 0, strlen(device_id)+1);
                    memcpy(g_callback_list.callback_node[g_callback_list.total_callbacks].device_id, device_id, strlen(device_id));
                }
            }
            g_callback_list.callback_node[g_callback_list.total_callbacks].version = version;
            g_callback_list.total_callbacks ++;
        }
    }
}

static WidgetBaseClass *get_widget_subscribe_callback(char * fulltopic)
{
    char topictmp[128] = {0};
    char device_id[32] = {0};

    for (int i = 0 ; i < g_callback_list.total_wcallbacks; i++) {
        memset(topictmp, 0, sizeof(topictmp));
        if( TOPIC_VERSION_V1 == g_callback_list.callback_node[i].version ) {
            if(g_callback_list.widget_callback_node[i].device_id == NULL) {
                HAL_PARAMS_Get_System_device_id(device_id, sizeof(device_id));
                sprintf(topictmp,"v1/%s/", device_id);
            } else {
                sprintf(topictmp,"v1/%s/", g_callback_list.widget_callback_node[i].device_id);
            }
        } else if( TOPIC_VERSION_V2 == g_callback_list.callback_node[i].version ) {
            if(g_callback_list.widget_callback_node[i].device_id == NULL) {
                HAL_PARAMS_Get_System_device_id(device_id, sizeof(device_id));
                if(PRODUCT_TYPE_GATEWAY == system_get_product_type()) {
                    sprintf(topictmp,"v2/gateway/%s/", device_id);
                } else {
                    sprintf(topictmp,"v2/device/%s/", device_id);
                }
            } else {
                if(PRODUCT_TYPE_GATEWAY == system_get_product_type()) {
                    sprintf(topictmp,"v2/gateway/%s/", g_callback_list.widget_callback_node[i].device_id);
                } else {
                    sprintf(topictmp,"v2/device/%s/", g_callback_list.widget_callback_node[i].device_id);
                }
            }
        }
        strcat(topictmp,g_callback_list.widget_callback_node[i].topic);
        if (strcmp(fulltopic, topictmp) == 0) {
            return g_callback_list.widget_callback_node[i].pWidgetBase;
        }
    }
    return NULL;
}

static void add_widget_subscibe_callback(topic_version_t version, char *topic, char *device_id, WidgetBaseClass *pWidgetBase, uint8_t qos)
{
    int if_found_topic = 0;
    int i = 0;

    if((NULL == topic) || (NULL == pWidgetBase)) {
        return;
    }

    for (i = 0 ; i < g_callback_list.total_wcallbacks; i++) {
        if (!strcmp(topic, g_callback_list.widget_callback_node[i].topic) && (version == g_callback_list.widget_callback_node[i].version)) {
            if(((NULL == device_id) && (NULL == g_callback_list.widget_callback_node[i].device_id))
                    || ((NULL != device_id) && (NULL != g_callback_list.widget_callback_node[i].device_id) && !strcmp(device_id, g_callback_list.widget_callback_node[i].device_id))) {
                if_found_topic = 1;
                break;
            }
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
            g_callback_list.widget_callback_node[g_callback_list.total_wcallbacks].topic = (char *)malloc(strlen(topic)+1);
            if(NULL !=g_callback_list.widget_callback_node[g_callback_list.total_wcallbacks].topic) {
                memset(g_callback_list.widget_callback_node[g_callback_list.total_wcallbacks].topic, 0, strlen(topic)+1);
                memcpy(g_callback_list.widget_callback_node[g_callback_list.total_wcallbacks].topic, topic, strlen(topic));
            }
            if(NULL != device_id) {
                g_callback_list.widget_callback_node[g_callback_list.total_wcallbacks].device_id = (char *)malloc(strlen(device_id)+1);
                if(NULL !=g_callback_list.widget_callback_node[g_callback_list.total_wcallbacks].device_id) {
                    memset(g_callback_list.widget_callback_node[g_callback_list.total_wcallbacks].device_id, 0, strlen(device_id)+1);
                    memcpy(g_callback_list.widget_callback_node[g_callback_list.total_wcallbacks].device_id, device_id, strlen(device_id));
                }
            }
            g_callback_list.widget_callback_node[g_callback_list.total_wcallbacks].version = version;
            g_callback_list.total_wcallbacks ++;
        }
    }
}

static void del_subscribe_callback(topic_version_t version, char * topic, char *device_id)
{
    if(NULL == topic) {
        return;
    }

    for (int i = 0 ; i < g_callback_list.total_callbacks; i++) {
        if (!strcmp(topic, g_callback_list.callback_node[i].topic) && (version == g_callback_list.callback_node[i].version)) {
            if(((NULL == device_id) && (NULL == g_callback_list.callback_node[i].device_id))
                    || ((NULL != device_id) && (NULL != g_callback_list.callback_node[i].device_id) && !strcmp(device_id, g_callback_list.callback_node[i].device_id))) {
                free(g_callback_list.callback_node[i].topic);
                free(g_callback_list.callback_node[i].device_id);
                memcpy(&g_callback_list.callback_node[i], &g_callback_list.callback_node[i+1], (g_callback_list.total_callbacks - 1 - i) * sizeof(struct CallBackNode));
                memset(&g_callback_list.callback_node[g_callback_list.total_callbacks-1], 0, sizeof(struct CallBackNode));
                g_callback_list.total_callbacks--;
                return;
            }
        }
    }

    for (int i = 0 ; i < g_callback_list.total_wcallbacks; i++) {
        if (!strcmp(topic, g_callback_list.widget_callback_node[i].topic) && (version == g_callback_list.widget_callback_node[i].version)) {
            if(((NULL == device_id) && (NULL == g_callback_list.widget_callback_node[i].device_id))
                    || ((NULL != device_id) && (NULL != g_callback_list.widget_callback_node[i].device_id) && !strcmp(device_id, g_callback_list.widget_callback_node[i].device_id))) {
                free(g_callback_list.widget_callback_node[i].topic);
                free(g_callback_list.widget_callback_node[i].device_id);
                memcpy(&g_callback_list.widget_callback_node[i], &g_callback_list.widget_callback_node[i+1], (g_callback_list.total_wcallbacks - 1 - i) * sizeof(struct WidgetCallBackNode));
                memset(&g_callback_list.widget_callback_node[g_callback_list.total_wcallbacks-1], 0, sizeof(struct WidgetCallBackNode));
                g_callback_list.total_wcallbacks--;
                return;
            }
        }
    }
}

static void resubscribe(void)
{
    for (int i = 0 ; i < g_callback_list.total_callbacks; i++) {
        intorobot_subscribe(g_callback_list.callback_node[i].version, g_callback_list.callback_node[i].topic, g_callback_list.callback_node[i].device_id,
                            g_callback_list.callback_node[i].callback, g_callback_list.callback_node[i].qos);
    }

    for (int i = 0 ; i < g_callback_list.total_wcallbacks; i++) {
        intorobot_widget_subscribe(g_callback_list.widget_callback_node[i].version, g_callback_list.widget_callback_node[i].topic, g_callback_list.widget_callback_node[i].device_id,
                            g_callback_list.widget_callback_node[i].pWidgetBase, g_callback_list.widget_callback_node[i].qos);
    }
}

static void mqtt_send_debug_info(void)
{
    uint32_t n;
    String s_debug_info="";

    for(n=0; (n<MQTT_MAX_PACKET_SIZE)&&(g_debug_tx_buffer.tail!=g_debug_tx_buffer.head); n++) {
        s_debug_info += (char)g_debug_tx_buffer.buffer[g_debug_tx_buffer.tail];
        g_debug_tx_buffer.tail = (unsigned int)(g_debug_tx_buffer.tail + 1) % CLOUD_DEBUG_BUFFER_SIZE;
    }

    if(n) {
        intorobot_publish(TOPIC_VERSION_V1, INTOROBOT_MQTT_SEND_DEBUG_TOPIC, (uint8_t *)s_debug_info.c_str(), n, 0, false);
        intorobot_publish(TOPIC_VERSION_V2, INTOROBOT_MQTT_DEBUGRX_TOPIC, (uint8_t *)s_debug_info.c_str(), n, 0, false);
    }
}

static void mqtt_receive_debug_info(uint8_t *pIn, uint32_t len)
{
    for(int n=0; n<len; n++) {
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
    if (INTOROBOT_CLOUD_CONNECTED) {
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

static void _cloud_params_init(void)
{
    char sv_domain[32] = {0}, http_domain[32] = {0}, dw_domain[38] = {0};

    if(0 == HAL_PARAMS_Get_System_sv_domain(sv_domain, sizeof(sv_domain))) {
        HAL_PARAMS_Set_System_sv_domain(INTOROBOT_SERVER_DOMAIN);
    }
    if(0 == HAL_PARAMS_Get_System_sv_port()) {
        HAL_PARAMS_Set_System_sv_port(INTOROBOT_SERVER_PORT);
    }
    if(0 == HAL_PARAMS_Get_System_http_domain(http_domain, sizeof(http_domain))) {
        HAL_PARAMS_Set_System_http_domain(INTOROBOT_HTTP_DOMAIN);
    }
    if(0 == HAL_PARAMS_Get_System_http_port()) {
        HAL_PARAMS_Set_System_http_port(INTOROBOT_HTTP_PORT);
    }
    if(0 == HAL_PARAMS_Get_System_dw_domain(dw_domain, sizeof(dw_domain))) {
        HAL_PARAMS_Set_System_dw_domain(INTOROBOT_UPDATE_DOMAIN);
    }
}

void intorobot_cloud_init(void)
{
    //对domain，port默认值处理发生变化，所以添加该函数初始化网络参数。
    _cloud_params_init();
    //mqtt server domain
    char sv_domain[32] = {0};
    HAL_PARAMS_Get_System_sv_domain(sv_domain, sizeof(sv_domain));
    //mqtt server port
    int sv_port = HAL_PARAMS_Get_System_sv_port();

    memset(&g_debug_tx_buffer, 0, sizeof(g_debug_tx_buffer));
    memset(&g_debug_rx_buffer, 0, sizeof(g_debug_rx_buffer));

    g_mqtt_client = MqttClientClass(sv_domain, sv_port, mqtt_client_callback, g_mqtt_tcp_client);

    // v2版本subscibe
    intorobot_subscribe(TOPIC_VERSION_V2, INTOROBOT_MQTT_ACTION_TOPIC, NULL, cloud_action_callback, 0);   //从平台获取系统控制信息
    intorobot_subscribe(TOPIC_VERSION_V2, INTOROBOT_MQTT_DEBUGTX_TOPIC, NULL, cloud_debug_callback, 0);   //从平台获取调试信息

    if(PRODUCT_TYPE_NOTE == system_get_product_type()) {
        intorobot_subscribe(TOPIC_VERSION_V2, INTOROBOT_MQTT_TX_TOPIC, NULL, cloud_data_receive_callback, 0); //从平台获取数据通讯信息

        // 添加默认数据点
        intorobotDefineDatapointBool(DPID_DEFAULT_BOOL_RESET, DP_PERMISSION_UP_DOWN, false, DP_POLICY_NONE, 0);           //reboot
        intorobotDefineDatapointBool(DPID_DEFAULT_BOOL_GETALLDATAPOINT, DP_PERMISSION_UP_DOWN, false, DP_POLICY_NONE, 0); //get all datapoint
    }
}

int intorobot_cloud_connect(void)
{
    MOLMC_LOGD(TAG, "---------mqtt connect start--------");
    //mqtt server domain
    char sv_domain[32] = {0}, http_domain[32] = {0}, dw_domain[38] = {0};
    HAL_PARAMS_Get_System_sv_domain(sv_domain, sizeof(sv_domain));
    HAL_PARAMS_Get_System_http_domain(http_domain, sizeof(http_domain));
    HAL_PARAMS_Get_System_dw_domain(dw_domain, sizeof(dw_domain));
    //mqtt server port
    int sv_port = HAL_PARAMS_Get_System_sv_port();
    g_mqtt_client.setServer(sv_domain, sv_port);
    if(g_keepAlive) {
        g_mqtt_client.setKeepAlive(g_keepAlive);
    }

    char device_id[38] = {0}, access_token[38] = {0};
    uint8_t access_token_hex[16] = {0};
    HAL_PARAMS_Get_System_device_id(device_id, sizeof(device_id));
    HAL_PARAMS_Get_System_access_token(access_token, sizeof(access_token));
    string2hex(access_token, access_token_hex, sizeof(access_token_hex), false);

    MOLMC_LOGD(TAG, "---------terminal params--------");
    MOLMC_LOGD(TAG, "mqtt domain     : %s", sv_domain);
    MOLMC_LOGD(TAG, "mqtt port       : %d", sv_port);
    MOLMC_LOGD(TAG, "http domain     : %s", http_domain);
    MOLMC_LOGD(TAG, "http port       : %d", HAL_PARAMS_Get_System_http_port());
    MOLMC_LOGD(TAG, "down domain     : %s", dw_domain);
    MOLMC_LOGD(TAG, "at_mode         : %d", HAL_PARAMS_Get_System_at_mode());
    MOLMC_LOGD(TAG, "zone            : %f", HAL_PARAMS_Get_System_zone());
    MOLMC_LOGD(TAG, "device_id       : %s", device_id);
    MOLMC_LOGD(TAG, "access_token    : %s", access_token);
    MOLMC_LOGD(TAG, "--------------------------------");

    String fulltopic, payload;
    fill_mqtt_topic(fulltopic, TOPIC_VERSION_V2, INTOROBOT_MQTT_WILL_TOPIC, NULL);

    int random_hex = random(INT_MAX);
    uint8_t ramdom_array[4], cMac_hex[16] = {0};
    char random_string[16] = {0}, cMac_string[33] = {0};

    ramdom_array[0] = ( random_hex >> 24 ) & 0xFF;
    ramdom_array[1] = ( random_hex >> 16 ) & 0xFF;
    ramdom_array[2] = ( random_hex >> 8 ) & 0xFF;
    ramdom_array[3] = ( random_hex ) & 0xFF;
    hex2string(ramdom_array, sizeof(ramdom_array), random_string, false);
    MqttConnectComputeCmac( (uint8_t *)random_string, strlen(random_string), access_token_hex, cMac_hex );
    hex2string(cMac_hex, 16, cMac_string, false);
    payload = random_string;
    payload += ':';
    if(System.featureEnabled(SYSTEM_FEATURE_CLOUD_DATA_ENCRYPT_ENABLED)) {
        payload += '1';
    } else {
        payload += '0';
    }
    payload += cMac_string;
    MOLMC_LOGD(TAG, "mqtt passwork ->  %s", payload.c_str());
    if(g_mqtt_client.connect(device_id, device_id, payload, fulltopic, 0, true, INTOROBOT_MQTT_WILL_MESSAGE)) {
        MqttConnectComputeSKeys( access_token_hex, random_hex, g_mqtt_nwkskey, g_mqtt_appskey );
        MOLMC_LOGD(TAG, "---------connect success--------");
        MOLMC_LOGD(TAG, "appskey -> ");
        MOLMC_LOGD_BUFFER_HEX(TAG, g_mqtt_appskey, 16);
        MOLMC_LOGD(TAG, "nwkskey -> ");
        MOLMC_LOGD_BUFFER_HEX(TAG, g_mqtt_nwkskey, 16);

        //重新订阅
        MOLMC_LOGD(TAG, "---------mqtt resubscribe--------");
        resubscribe();
        //发送info
        if(System.featureEnabled(SYSTEM_FEATURE_SEND_INFO_ENABLED)) {
            aJsonClass aJson;
            char buffer[33] = {0};

            aJsonObject *root = aJson.createObject();
            if (root == NULL) {
                return -1;
            }
            system_get_product_id(buffer, sizeof(buffer));
            aJson.addStringToObject(root, "productId", buffer);

            aJson.addNumberToObject(root, "bc", 1);
            system_get_board_id(buffer, sizeof(buffer));
            aJson.addStringToObject(root, "board", buffer);
            system_get_product_software_version(buffer, sizeof(buffer));
            aJson.addStringToObject(root, "swVer", buffer);
            system_get_product_hardware_version(buffer, sizeof(buffer));
            aJson.addStringToObject(root, "hwVer", buffer);

            //后期将废除
            HAL_PARAMS_Get_System_fwlib_ver(buffer, sizeof(buffer));
            aJson.addStringToObject(root, "libVer", buffer);

            aJson.addBooleanToObject(root, "online", true);
            char *string = aJson.print(root);
            //MOLMC_LOGD(TAG, "info = %s", string);
            intorobot_publish(TOPIC_VERSION_V2, INTOROBOT_MQTT_WILL_TOPIC, (uint8_t*)string, strlen(string), 0, true);
            free(string);
            aJson.deleteItem(root);
        }
        return 0;
    }
    MOLMC_LOGD(TAG, "---------connect failed--------");
    return -1;
}

int intorobot_cloud_handle(void)
{
    if(true == g_mqtt_client.loop()) {
        intorobotSendDatapointAutomatic();
        mqtt_send_debug_info(); //发送IntoRobot.printf打印到平台
        return 0;
    }
    return -1;
}

void intorobot_cloud_keepalive(uint16_t sec)
{
    g_keepAlive = sec;
}

bool intorobot_sync_time(void)
{
    MOLMC_LOGD(TAG, "---------device syncTime begin---------");
    time_t utc_time = 0;
    bool flag = false;
    HTTPClient http;
    aJsonClass aJson;
    String payload = "";

    char http_domain[32] = {0};
    HAL_PARAMS_Get_System_http_domain(http_domain, sizeof(http_domain));
    int http_port = HAL_PARAMS_Get_System_http_port();

    http.begin(http_domain, http_port, "/v1/device?act=getts");
    http.setUserAgent(F("User-Agent: Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.101 Safari/537.36"));
    int httpCode = http.POST(payload);
    if(httpCode == HTTP_CODE_OK) {
        payload = http.getString();
        //MOLMC_LOGD(TAG, "payload    : %s", (char *)payload.c_str());
        aJsonObject* root = aJson.parse((char *)payload.c_str());
        if (root == NULL)
        {return false;}

        aJsonObject *tsObject = aJson.getObjectItem(root, "ts");
        if (tsObject != NULL) {
            if( tsObject->type == aJson_Int ) {
                utc_time = tsObject->valueint;
            } else if( tsObject->type == aJson_Uint ) {
                utc_time = tsObject->valueuint;
            }
            MOLMC_LOGD(TAG, "utc_time    : %d", utc_time);
            randomSeed(utc_time);
            Time.setTime(utc_time);
            flag = true;
        }
        aJson.deleteItem(root);
    }
    http.end();
    if(flag) {
        return true;
    } else {
        MOLMC_LOGD(TAG, "device syncTime failed!");
        return false;
    }
}

bool intorobot_device_register(char *prodcut_id, time_t utc_time, char *signature)
{
    MOLMC_LOGD(TAG, "---------device register begin---------");

    HTTPClient http;
    aJsonClass aJson;
    bool flag = false;

    aJsonObject* root = aJson.createObject();
    if (root == NULL)
    {return false;}

    //获取product id
    aJson.addStringToObject(root, "productId", prodcut_id);
    aJson.addStringToObject(root, "timestamp", String(utc_time).c_str());
    aJson.addStringToObject(root, "signature", signature);
    char* string = aJson.print(root);
    String payload = string;
    free(string);
    aJson.deleteItem(root);

    char http_domain[32] = {0};
    HAL_PARAMS_Get_System_http_domain(http_domain, sizeof(http_domain));
    int http_port = HAL_PARAMS_Get_System_http_port();

    http.begin(http_domain, http_port, "/v1/device?act=register");
    http.setUserAgent(F("User-Agent: Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.101 Safari/537.36"));
    int httpCode = http.POST(payload);
    if(httpCode == HTTP_CODE_OK) {
        payload = http.getString();
        root = aJson.parse((char *)payload.c_str());
        if (root == NULL)
        {return false;}

        //device_id  and access_token
        aJsonObject *deviceIdObject = aJson.getObjectItem(root, "deviceId");
        aJsonObject *accessTokenObject = aJson.getObjectItem(root, "token");
        if ( deviceIdObject != NULL && accessTokenObject != NULL) {
            HAL_PARAMS_Set_System_device_id(deviceIdObject->valuestring);
            HAL_PARAMS_Set_System_access_token(accessTokenObject->valuestring);
            HAL_PARAMS_Set_System_at_mode(AT_MODE_FLAG_ABP);
            HAL_PARAMS_Save_Params();
            MOLMC_LOGD(TAG, "device_id    : %s", deviceIdObject->valuestring);
            MOLMC_LOGD(TAG, "access_token : %s", accessTokenObject->valuestring);
            MOLMC_LOGD(TAG, "device register success!");
            flag = true;
        }
        aJson.deleteItem(root);
    }
    http.end();
    if(flag) {
        return true;
    } else {
        MOLMC_LOGD(TAG, "device register failed!");
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
    char device_id[32] = {0};

    HAL_PARAMS_Get_System_device_id(device_id, sizeof(device_id));
    return device_id;
}

volatile uint8_t intorobot_process_flag = 0;
void intorobot_process(void)
{
    if(intorobot_process_flag) {
        return;
    }

    intorobot_process_flag = 1;

#if FIRMWARE_CONFIG_SYSTEM_SETUP
    if(SYSTEM_CONFIG_TYPE_NONE != get_system_config_type()) {
        manage_system_config();
        goto exit_process;
    }
#endif

    // application thread will pump application messages
#if PLATFORM_THREADING
    if (system_thread_get_state(NULL) && APPLICATION_THREAD_CURRENT()) {
        ApplicationThread.process();
        goto exit_process;
    }
#endif

    // run the background processing loop, and specifically also pump cloud events
    system_process_loop();
    HAL_Core_System_Yield();

exit_process:
    intorobot_process_flag = 0;
}

