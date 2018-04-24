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
#ifdef configSETUP_ENABLE

#include <stdio.h>
#include <stdlib.h>
#include "system_config.h"
#include "system_rgbled.h"
#include "system_cloud_def.h"
#include "system_cloud.h"
#include "system_lorawan.h"
#include "system_network.h"
#include "params_hal.h"
#include "core_hal.h"
#include "inet_hal.h"
#include "wlan_hal.h"
#include "wiring_wifi.h"
#include "wiring_cellular.h"
#include "wiring_constants.h"
#include "wiring_ticks.h"
#include "wiring_system.h"
#include "string_convert.h"
#include "system_mode.h"
#include "system_task.h"
#include "system_test.h"
#include "system_utilities.h"
#include "system_event.h"
#include "wiring_rgb.h"

/*debug switch*/
#define SYSTEM_CONFIG_DEBUG

#ifdef SYSTEM_CONFIG_DEBUG
#define SCONFIG_DEBUG(...)    do {DEBUG(__VA_ARGS__);}while(0)
#define SCONFIG_DEBUG_D(...)  do {DEBUG_D(__VA_ARGS__);}while(0)
#define SCONFIG_DEBUG_DUMP    DEBUG_DUMP
#else
#define SCONFIG_DEBUG(...)
#define SCONFIG_DEBUG_D(...)
#define SCONFIG_DEBUG_DUMP
#endif

#ifndef configNO_NETWORK
//using namespace intorobot;
#endif
using namespace intorobot;

typedef size_t (*writeHandler)(const uint8_t *buf, size_t size);

static volatile uint32_t config_timeout_start;
static volatile uint32_t config_timeout_duration;

inline void ARM_CONFIG_TIMEOUT(uint32_t dur) {
    config_timeout_start = HAL_Timer_Get_Milli_Seconds();
    config_timeout_duration = dur;
    SCONFIG_DEBUG("CONFIG WD Set %d\r\n",(dur));
}
inline bool IS_CONFIG_TIMEOUT() {
    return config_timeout_duration && ((HAL_Timer_Get_Milli_Seconds()-config_timeout_start)>config_timeout_duration);
}

inline void CLR_CONFIG_TIMEOUT() {
    config_timeout_duration = 0;
    SCONFIG_DEBUG("CONFIG WD Cleared, was %d\r\n", config_timeout_duration);
}

DeviceConfigCmdType DeviceConfig::getMessageType(char *s) {
    if(!strcmp(s,"hello")) {
        return DEVICE_CONFIG_HELLO;
    } else if(!strcmp(s,"checkWifi")) {
        return DEVICE_CONFIG_CHECK_WIFI;
    } else if(!strcmp(s,"getNetworkStatus")) {
        return DEVICE_CONFIG_GET_NETWORK_STATUS;
    } else if(!strcmp(s,"getWifiList")) {
        return DEVICE_CONFIG_GET_WIFI_LIST;
    } else if(!strcmp(s,"getInfo")) {
        return DEVICE_CONFIG_GET_INFO;
    } else if(!strcmp(s,"sendWifiInfo")) {
        return DEVICE_CONFIG_SEND_WIFI_INFO;
    } else if(!strcmp(s,"setNetworkCredentials")) {
        return DEVICE_CONFIG_SET_NETWORK_CREDENTIALS;
    } else if(!strcmp(s,"sendDeviceInfo")) {
        return DEVICE_CONFIG_SET_DEVICE_INFO;
    } else if(!strcmp(s,"setSecurity")) {
        return DEVICE_CONFIG_SET_SECURITY;
    } else if(!strcmp(s,"setInfo")) {
        return DEVICE_CONFIG_SET_INFO;
    } else if(!strcmp(s,"restartNetwork")) {
        return DEVICE_CONFIG_RESTART_NETWORK;
    } else if(!strcmp(s,"reset")) {
        return DEVICE_CONFIG_RESET;
    } else if(!strcmp(s,"reboot")) {
        return DEVICE_CONFIG_REBOOT;
    } else if(!strcmp(s,"exit")) {
        return DEVICE_CONFIG_EXIT;
    } else if(!strcmp(s,"test")) {
        return DEVICE_CONFIG_TEST;
    } else {
        return DEVICE_CONFIG_ERROR;
    }
}

bool DeviceConfig::process(void)
{
    aJsonObject *root = NULL;
    bool _isConfigSuccessful = false;

    while(available()) {
        String tmp=readString();

        //SCONFIG_DEBUG("OK! Rev: %s\r\n", (char *)tmp.c_str());
        root = aJson.parse((char *)tmp.c_str());
        if (root == NULL) {break;}

        aJsonObject* command_Object = aJson.getObjectItem(root, "command");
        if (command_Object == NULL) {break;}

        DeviceConfigCmdType type = getMessageType(command_Object->valuestring);
        switch(type) {
            //查询类指令
            case DEVICE_CONFIG_HELLO:                   //获取设备基础信息
                aJson.deleteItem(root);
                dealHello();
                break;
            case DEVICE_CONFIG_CHECK_WIFI:              //获取wifi状态
                aJson.deleteItem(root);
                dealCheckWifi();
                break;
            case DEVICE_CONFIG_GET_WIFI_LIST:           //获取wifi列表
                aJson.deleteItem(root);
                dealGetWifiList();
                break;
            case DEVICE_CONFIG_GET_NETWORK_STATUS:      //查询网络状态
                aJson.deleteItem(root);
                dealGetNetworkStatus();
                break;
            case DEVICE_CONFIG_GET_INFO:                //获取设备信息
                aJson.deleteItem(root);
                dealGetInfo();
                break;
            //设置类指令
            case DEVICE_CONFIG_SEND_WIFI_INFO:          //设置wifi信息
            case DEVICE_CONFIG_SET_NETWORK_CREDENTIALS: //设置网络接入凭证
                dealSetNetworkCredentials(root);
                break;
            case DEVICE_CONFIG_SET_DEVICE_INFO:         //设置设备信息
                dealSendDeviceInfo(root);
                _isConfigSuccessful = true;
                close();
                break;
            case DEVICE_CONFIG_SET_SECURITY:            //设置设备安全信息
                dealSetSecurity(root);
                break;
            case DEVICE_CONFIG_SET_INFO:                //设置设备信息
                dealSetInfo(root);
                break;
            //执行类指令
            case DEVICE_CONFIG_RESTART_NETWORK:         //重启网络
                aJson.deleteItem(root);
                dealRestartNetwork();
                break;
            case DEVICE_CONFIG_RESET:                   //设备恢复出厂设置
                aJson.deleteItem(root);
                dealReset();
                break;
            case DEVICE_CONFIG_REBOOT:                  //设备重启
                aJson.deleteItem(root);
                dealReboot();
                break;
            case DEVICE_CONFIG_EXIT:                    //退出配置模式
                aJson.deleteItem(root);
                sendComfirm(200);
                _isConfigSuccessful = true;
                close();
                break;
            //测试类指令
            case DEVICE_CONFIG_TEST:                    //设备测试
                dealTest(root);
                break;
            case DEVICE_CONFIG_ERROR: //错误
                aJson.deleteItem(root);
                sendComfirm(201);
                break;
            default:
                break;
        }
        if(_isConfigSuccessful) {break;}
    }
    return _isConfigSuccessful ? true : false;
}

void DeviceConfig::sendComfirm(int status)
{
    aJsonObject* root = aJson.createObject();
    if (root == NULL) {return;}

    aJson.addNumberToObject(root, "status", status);
    char* string = aJson.print(root);
    if(NULL != string) {
        write((unsigned char *)string, strlen(string));
        free(string);
    }
    aJson.deleteItem(root);
}

void DeviceConfig::dealHello(void)
{
    aJsonObject* root = aJson.createObject();
    if (root == NULL) {return;}

    aJson.addNumberToObject(root, "status", 200);
    aJson.addNumberToObject(root, "version", 2);  //v2版本配置协议
    char device_id[32]="", board[32]="";
    system_get_board_id(board, sizeof(board));
    aJson.addStringToObject(root, "board", (char *)board);
    if (AT_MODE_FLAG_NONE != HAL_PARAMS_Get_System_at_mode()) {
        HAL_PARAMS_Get_System_device_id(device_id, sizeof(device_id));
        aJson.addStringToObject(root, "device_id", (char *)device_id);
    }
    aJson.addNumberToObject(root, "at_mode", HAL_PARAMS_Get_System_at_mode());
    char* string = aJson.print(root);
    if(NULL != string) {
        write((unsigned char *)string, strlen(string));
        free(string);
    }
    aJson.deleteItem(root);
}

void DeviceConfig::dealCheckWifi(void)
{
#ifdef configWIRING_WIFI_ENABLE
    aJsonObject* root = aJson.createObject();
    if (root == NULL) {return;}

    if(WiFi.ready()) {
        manage_ip_config();
        aJson.addNumberToObject(root, "status", 200);
        aJson.addStringToObject(root, "ssid", WiFi.SSID());
        aJson.addNumberToObject(root, "rssi", WiFi.RSSI());
    } else {
        aJson.addNumberToObject(root, "status", 201);
    }
    char* string = aJson.print(root);
    if(NULL != string) {
        write((unsigned char *)string, strlen(string));
        free(string);
    }
    aJson.deleteItem(root);
#elif (defined configWIRING_CELLULAR_ENABLE) || (defined configWIRING_LORA_ENABLE)
    sendComfirm(200);
#endif
}

void DeviceConfig::dealGetNetworkStatus(void)
{
#ifdef configWIRING_WIFI_ENABLE
    dealCheckWifi();
#elif defined configWIRING_CELLULAR_ENABLE
    sendComfirm(200);
#elif defined configWIRING_LORA_ENABLE
    sendComfirm(200);
#endif
}

void DeviceConfig::dealGetWifiList(void)
{
#ifdef configWIRING_WIFI_ENABLE
    WiFiAccessPoint ap[10];

    wlan_Imlink_stop();
    int found = WiFi.scan(ap, 10);
    if(found < 1) {
        sendComfirm(201);
    } else {
        aJsonObject* root = aJson.createObject();
        if (root == NULL) {return;}
        aJson.addNumberToObject(root, "status",200);
        aJson.addNumberToObject(root, "listnum",found);
        aJsonObject* ssidlistarray = aJson.createArray();
        if (ssidlistarray == NULL) {
            aJson.deleteItem(root);
            return;
        }
        aJson.addItemToObject(root, "ssidlist", ssidlistarray);

        for(int n = 0; n < found; n++)
        {
            aJsonObject* ssid_object = aJson.createObject();
            if (ssid_object == NULL) {
                aJson.deleteItem(root);
                return;
            }
            aJson.addItemToArray(ssidlistarray, ssid_object);
            aJson.addStringToObject(ssid_object, "ssid", ap[n].ssid);
            aJson.addNumberToObject(ssid_object, "entype", ap[n].security);
            aJson.addNumberToObject(ssid_object, "signal", ap[n].rssi);
        }
        char* string = aJson.print(root);
        if(NULL != string) {
            write((unsigned char *)string, strlen(string));
            free(string);
        }
        aJson.deleteItem(root);
    }
    wlan_Imlink_start();
#elif (defined configWIRING_CELLULAR_ENABLE) || (defined configWIRING_LORA_ENABLE)
    sendComfirm(201);
#endif
}

void DeviceConfig::dealGetInfo(void)
{
    DEBUG("dealGetInfo = %d\r\n", System.freeMemory());
    aJsonObject* root = aJson.createObject();
    if (root == NULL) {return;}

    aJson.addNumberToObject(root, "status", 200);
    aJsonObject* value_Object = aJson.createObject();
    if (value_Object == NULL) {return;}

    char device_id[32]="",board[32]="";
    system_get_board_id(board, sizeof(board));
    aJson.addStringToObject(value_Object, "board", board);
    HAL_PARAMS_Get_System_device_id(device_id, sizeof(device_id));
    aJson.addStringToObject(value_Object, "device_id", device_id);
    aJson.addNumberToObject(value_Object, "at_mode", HAL_PARAMS_Get_System_at_mode());

    aJson.addNumberToObject(value_Object, "zone", HAL_PARAMS_Get_System_zone());

#if (defined configWIRING_WIFI_ENABLE) || (defined configWIRING_CELLULAR_ENABLE)
    char domain[50] = {0};
    HAL_PARAMS_Get_System_sv_domain(domain, sizeof(domain));
    aJson.addStringToObject(value_Object, "sv_domain", domain);
    aJson.addNumberToObject(value_Object, "sv_port", HAL_PARAMS_Get_System_sv_port());

    memset(domain, 0, sizeof(domain));
    HAL_PARAMS_Get_System_http_domain(domain, sizeof(domain));
    aJson.addStringToObject(value_Object, "http_domain", domain);
    aJson.addNumberToObject(value_Object, "http_port", HAL_PARAMS_Get_System_http_port());

    memset(domain, 0, sizeof(domain));
    HAL_PARAMS_Get_System_dw_domain(domain, sizeof(domain));
    aJson.addStringToObject(value_Object, "dw_domain", domain);

#ifdef configWIRING_WIFI_ENABLE
    uint8_t stamac[6] = {0}, apmac[6] = {0};
    char macStr[20] = {0};

    wlan_get_macaddr(stamac, apmac);
    memset(macStr, 0, sizeof(macStr));
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", stamac[0], stamac[1], stamac[2], stamac[3], stamac[4], stamac[5]);
    aJson.addStringToObject(value_Object, "stamac", macStr);
    memset(macStr, 0, sizeof(macStr));
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", apmac[0], apmac[1], apmac[2], apmac[3], apmac[4], apmac[5]);
    aJson.addStringToObject(value_Object, "apmac", macStr);
#endif
#elif defined configWIRING_LORA_ENABLE
    char devaddr[12];
    HAL_PARAMS_Get_System_devaddr(devaddr, sizeof(devaddr));
    aJson.addStringToObject(value_Object, "devaddr", devaddr);
#endif
    aJson.addItemToObject(root, "value", value_Object);
    char* string = aJson.print(root);
    if(NULL != string) {
        write((unsigned char *)string, strlen(string));
        free(string);
    }
    aJson.deleteItem(root);
}

void DeviceConfig::dealSetNetworkCredentials(aJsonObject* root)
{
    aJsonObject* value_Object = aJson.getObjectItem(root, "value");
    if (value_Object == NULL) {
        aJson.deleteItem(root);
        sendComfirm(201);
        return;
    }

#ifdef configWIRING_WIFI_ENABLE
    wlan_Imlink_stop();
    network_disconnect(0, 0, NULL);
    network_connect(0, 0, 0, NULL);

    aJsonObject* ssidObject = aJson.getObjectItem(value_Object, "ssid");
    aJsonObject* passwdObject = aJson.getObjectItem(value_Object, "passwd");
    if ((ssidObject != NULL) && (passwdObject != NULL)) {
        if(0==strcmp(ssidObject->valuestring,"")) { //密码为空
            WiFi.setCredentials(ssidObject->valuestring);
        } else {
            WiFi.setCredentials(ssidObject->valuestring, passwdObject->valuestring);
        }
        sendComfirm(200);
        return;
    }
    aJson.deleteItem(root);
    sendComfirm(201);
#elif (defined configWIRING_CELLULAR_ENABLE) || (defined configWIRING_LORA_ENABLE)
    aJson.deleteItem(root);
    sendComfirm(201);
#endif
}

void DeviceConfig::dealSendDeviceInfo(aJsonObject* root)
{
    aJsonObject* value_Object = aJson.getObjectItem(root, "value");
    if (value_Object == NULL) {
        aJson.deleteItem(root);
        sendComfirm(201);
        return;
    }

#ifdef configWIRING_WIFI_ENABLE
    //zone
    aJsonObject* zoneObject = aJson.getObjectItem(value_Object, "zone");
    if (zoneObject != NULL) {
        float valuefloat = (zoneObject->type==aJson_Int?(float)(zoneObject->valueint):zoneObject->valuefloat);
        if(valuefloat < -12 || valuefloat > 13)
        {valuefloat = 8.0;}
        HAL_PARAMS_Set_System_zone(valuefloat);
    }
    //device_id  and access_token
    aJsonObject* devieIdObject = aJson.getObjectItem(value_Object, "device_id");
    aJsonObject* accessTokenObject = aJson.getObjectItem(value_Object, "access_token");
    if (devieIdObject != NULL && accessTokenObject != NULL) {
        HAL_PARAMS_Set_System_device_id(devieIdObject->valuestring);
        HAL_PARAMS_Set_System_access_token(accessTokenObject->valuestring);
        HAL_PARAMS_Set_System_at_mode(AT_MODE_FLAG_ABP);
    }
    //mqtt server domain
    aJsonObject* svDomainObject = aJson.getObjectItem(value_Object, "sv_domain");
    if (svDomainObject != NULL) {
        HAL_PARAMS_Set_System_sv_domain(svDomainObject->valuestring);
    }
    //mqtt server port
    aJsonObject* svPortObject = aJson.getObjectItem(value_Object, "sv_port");
    if (svPortObject != NULL) {
        HAL_PARAMS_Set_System_sv_port(svPortObject->valueint);
    }
    //down server domain
    aJsonObject* dwDomainObject = aJson.getObjectItem(value_Object, "dw_domain");
    if (dwDomainObject != NULL) {
        HAL_PARAMS_Set_System_dw_domain(dwDomainObject->valuestring);
    }
    HAL_PARAMS_Save_Params();
    aJson.deleteItem(root);
    sendComfirm(200);
#elif (defined configWIRING_CELLULAR_ENABLE) || (defined configWIRING_LORA_ENABLE)
    aJson.deleteItem(root);
    sendComfirm(200);
#endif
}

void DeviceConfig::dealSetSecurity(aJsonObject* root)
{
    bool flag = false;

    aJsonObject* value_Object = aJson.getObjectItem(root, "value");
    if (value_Object == NULL) {
        aJson.deleteItem(root);
        sendComfirm(201);
        return;
    }

    aJsonObject *deviceIdObject = aJson.getObjectItem(value_Object, "device_id");
    //at_mode
    aJsonObject *atModeObject = aJson.getObjectItem(value_Object, "at_mode");
    if (atModeObject != NULL) {
#if (defined configWIRING_WIFI_ENABLE) || (defined configWIRING_CELLULAR_ENABLE)
        aJsonObject *accessTokenObject = aJson.getObjectItem(value_Object, "access_token");
        if(AT_MODE_FLAG_ABP == atModeObject->valueint) {
            if (deviceIdObject != NULL && accessTokenObject != NULL) {
                HAL_PARAMS_Set_System_device_id(deviceIdObject->valuestring);
                HAL_PARAMS_Set_System_access_token(accessTokenObject->valuestring);
                flag = true;
            }
        }
#elif defined configWIRING_LORA_ENABLE
        aJsonObject *appeuiObject = aJson.getObjectItem(value_Object, "appeui");
        aJsonObject *appkeyObject = aJson.getObjectItem(value_Object, "appkey");
        aJsonObject *devaddrObject = aJson.getObjectItem(value_Object, "devaddr");
        aJsonObject *nwkskeyObject = aJson.getObjectItem(value_Object, "nwkskey");
        aJsonObject *appskeyObject = aJson.getObjectItem(value_Object, "appskey");

        if(AT_MODE_FLAG_ABP == atModeObject->valueint) {
            if (deviceIdObject != NULL && devaddrObject != NULL && nwkskeyObject != NULL \
                    && appskeyObject != NULL) {
                HAL_PARAMS_Set_System_device_id(deviceIdObject->valuestring);
                HAL_PARAMS_Set_System_devaddr(devaddrObject->valuestring);
                HAL_PARAMS_Set_System_nwkskey(nwkskeyObject->valuestring);
                HAL_PARAMS_Set_System_appskey(appskeyObject->valuestring);
                flag = true;
            }
        } else if(AT_MODE_FLAG_OTAA_INACTIVE == atModeObject->valueint) {
            if (deviceIdObject != NULL && appeuiObject != NULL && appkeyObject != NULL) {
                HAL_PARAMS_Set_System_device_id(deviceIdObject->valuestring);
                HAL_PARAMS_Set_System_appeui(appeuiObject->valuestring);
                HAL_PARAMS_Set_System_appkey(appkeyObject->valuestring);
                flag = true;
            }
        } else if(AT_MODE_FLAG_OTAA_ACTIVE == atModeObject->valueint) {
            if (deviceIdObject != NULL && appeuiObject != NULL && appkeyObject != NULL \
                    && devaddrObject != NULL && nwkskeyObject != NULL && appskeyObject != NULL) {
                HAL_PARAMS_Set_System_device_id(deviceIdObject->valuestring);
                HAL_PARAMS_Set_System_appeui(appeuiObject->valuestring);
                HAL_PARAMS_Set_System_appkey(appkeyObject->valuestring);
                HAL_PARAMS_Set_System_devaddr(devaddrObject->valuestring);
                HAL_PARAMS_Set_System_nwkskey(nwkskeyObject->valuestring);
                HAL_PARAMS_Set_System_appskey(appskeyObject->valuestring);
                flag = true;
            }
        }
#endif
    }
    if(true == flag) {
        HAL_PARAMS_Set_System_at_mode((AT_MODE_FLAG_TypeDef)atModeObject->valueint);
        HAL_PARAMS_Save_Params();
        aJson.deleteItem(root);
        sendComfirm(200);
    } else {
        aJson.deleteItem(root);
        sendComfirm(201);
    }
}

void DeviceConfig::dealSetInfo(aJsonObject* root)
{
    bool flag = true;
    aJsonObject* value_Object = aJson.getObjectItem(root, "value");
    if (value_Object == NULL) {
        aJson.deleteItem(root);
        sendComfirm(201);
        return;
    }

    //zone
    aJsonObject* zoneObject = aJson.getObjectItem(value_Object, "zone");
    if (zoneObject != NULL) {
        float valuefloat = (zoneObject->type==aJson_Int?(float)(zoneObject->valueint):zoneObject->valuefloat);
        if(valuefloat < -12 || valuefloat > 13)
        {valuefloat = 8.0;}
        HAL_PARAMS_Set_System_zone(valuefloat);
    }

#if (defined configWIRING_WIFI_ENABLE) || (defined configWIRING_CELLULAR_ENABLE)
    //mqtt server domain
    aJsonObject* svDomainObject = aJson.getObjectItem(value_Object, "sv_domain");
    if (svDomainObject != NULL) {
        HAL_PARAMS_Set_System_sv_domain(svDomainObject->valuestring);
    }
    //mqtt server port
    aJsonObject* svPortObject = aJson.getObjectItem(value_Object, "sv_port");
    if (svPortObject != NULL) {
        HAL_PARAMS_Set_System_sv_port(svPortObject->valueint);
    }
    //down server domain
    aJsonObject* dwDomainObject = aJson.getObjectItem(value_Object, "dw_domain");
    if (dwDomainObject != NULL) {
        HAL_PARAMS_Set_System_dw_domain(dwDomainObject->valuestring);
    }
    //http server domain
    aJsonObject* httpDomainObject = aJson.getObjectItem(value_Object, "http_domain");
    if (httpDomainObject != NULL) {
        HAL_PARAMS_Set_System_http_domain(httpDomainObject->valuestring);
    }
    //http server port
    aJsonObject* httpPortObject = aJson.getObjectItem(value_Object, "http_port");
    if (httpPortObject != NULL) {
        HAL_PARAMS_Set_System_http_port(httpPortObject->valueint);
    }
#ifdef configWIRING_WIFI_ENABLE
    uint8_t stamac[6] = {0}, apmac[6] = {0};
    aJsonObject* stamacObject = aJson.getObjectItem(value_Object, "stamac");
    aJsonObject* apmacObject = aJson.getObjectItem(value_Object, "apmac");
    if ((stamacObject != NULL) && (apmacObject != NULL)) {
        mac_str_to_bin(stamacObject->valuestring, stamac);
        mac_str_to_bin(apmacObject->valuestring, apmac);
        if(wlan_set_macaddr(stamac, apmac)) {
            flag = false;
        }
    }
#endif
#endif
    if(true == flag) {
        HAL_PARAMS_Save_Params();
        aJson.deleteItem(root);
        sendComfirm(200);
    } else {
        aJson.deleteItem(root);
        sendComfirm(201);
    }
}

void DeviceConfig::dealRestartNetwork(void)
{
#ifdef configWIRING_WIFI_ENABLE
    WiFi.off();
    delay(1000);
    WiFi.on();
    sendComfirm(200);
#elif (defined configWIRING_CELLULAR_ENABLE) || (defined configWIRING_LORA_ENABLE)
    sendComfirm(200);
#endif
}

void DeviceConfig::dealReset(void)
{
    HAL_Core_Enter_Factory_Reset_Mode();
    sendComfirm(200);
}

void DeviceConfig::dealReboot(void)
{
    HAL_Core_System_Reset();
    sendComfirm(200);
}

void DeviceConfig::dealExit(void)
{
    sendComfirm(200);
}

void DeviceConfig::dealTest(aJsonObject* root)
{
    aJsonObject* value_Object = aJson.getObjectItem(root, "value");
    if (value_Object == NULL) {
        aJson.deleteItem(root);
        sendComfirm(201);
        return;
    }

#if (defined configWIRING_WIFI_ENABLE) || (defined configWIRING_CELLULAR_ENABLE)
    uint16_t pinNum;
    uint8_t pinLevel;

    aJsonObject* itemObject = aJson.getObjectItem(value_Object, "item");
    if(itemObject == NULL) {return;}

    if(strcmp(itemObject->valuestring,"digitalWrite") == 0) {
        aJsonObject* pinObject = aJson.getObjectItem(value_Object,"pin");
        if(pinObject == NULL) {return;}
        pinNum = pinObject->valueint;

        aJsonObject* valObject = aJson.getObjectItem(value_Object,"val");
        if(valObject == NULL) {return;}
        if(strcmp(valObject->valuestring,"HIGH") == 0) {
            pinLevel = HIGH;
        } else {
            pinLevel = LOW;
        }
        aJson.deleteItem(root);
        testDigitalWrite(pinNum,pinLevel,this);
    } else if(strcmp(itemObject->valuestring,"analogRead") == 0) {
        aJsonObject* pinObject = aJson.getObjectItem(value_Object,"pin");
        if(pinObject == NULL) {return;}
        pinNum = pinObject->valueint;
        aJson.deleteItem(root);
        testAnalogRead(pinNum,this);
    } else if(strcmp(itemObject->valuestring,"selfTest") == 0) {
        aJson.deleteItem(root);
        testSelfTest(this);
    } else if(strcmp(itemObject->valuestring,"rfCheck") == 0) {
        aJson.deleteItem(root);
        testRfCheck(this);
    } else if(strcmp(itemObject->valuestring,"sensorData") == 0) {
        aJson.deleteItem(root);
        testSensorData(this);
    }
#endif
}

#ifdef configSETUP_USBSERIAL_ENABLE
void UsbDeviceConfig::init(void)
{
    serialusb.setTimeout(50);
    serialusb.begin(115200);
    while (!serialusb);
}

int UsbDeviceConfig::available(void)
{
    return serialusb.available();
}

int UsbDeviceConfig::read(void)
{
    return serialusb.read();
}

String UsbDeviceConfig::readString(void)
{
    return serialusb.readString();
}

size_t UsbDeviceConfig::write(const uint8_t *buffer, size_t size)
{
    for(size_t i = 0;i<size;i++) {
        serialusb.write(buffer[i]);
    }
    return size;
}

void UsbDeviceConfig::sendComfirm(int status)
{
    aJsonObject* root = aJson.createObject();
    if (root == NULL)
    {return;}

    aJson.addNumberToObject(root, "status", status);
    char* string = aJson.print(root);
    if(NULL != string) {
        write((unsigned char *)string, strlen(string));
        free(string);
    }
    aJson.deleteItem(root);
}

void UsbDeviceConfig::close(void)
{
    //none do
}
#endif

#ifdef configSETUP_USARTSERIAL_ENABLE
void UsartDeviceConfig::init(void)
{
    serial.setTimeout(50);
    serial.end();
    serial.begin(115200);
    while (!serial);
}

int UsartDeviceConfig::available(void)
{
    return serial.available();
}

int UsartDeviceConfig::read(void)
{
    return serial.read();
}

String UsartDeviceConfig::readString(void)
{
    return serial.readString();
}

size_t UsartDeviceConfig::write(const uint8_t *buffer, size_t size)
{
    for(size_t i = 0;i<size;i++) {
        serial.write(buffer[i]);
    }
    return size;
}

void UsartDeviceConfig::sendComfirm(int status)
{
    aJsonObject* root = aJson.createObject();
    if (root == NULL)
    {return;}

    aJson.addNumberToObject(root, "status", status);
    char* string = aJson.print(root);
    if(NULL != string) {
        write((unsigned char *)string, strlen(string));
        free(string);
    }
    aJson.deleteItem(root);
}

void UsartDeviceConfig::close(void)
{
    //none do
}
#endif

#ifdef configSETUP_TCP_ENABLE
void TcpDeviceConfig::init(void)
{
    server.begin();
}

void TcpDeviceConfig::sendComfirm(int status)
{
    aJsonObject* root = aJson.createObject();
    if (root == NULL)
    {return;}

    aJson.addNumberToObject(root, "status", status);
    char* string = aJson.print(root);
    if(NULL != string) {
        for(int i=0; i < 10; i++) //may be not enough
        {
            write((unsigned char *)string, strlen(string));
            delay(100);
        }
        free(string);
    }
    aJson.deleteItem(root);
}

int TcpDeviceConfig::available(void)
{
    client_bak = server.available();
    if(client_bak) {
        if(client) {
            client.stop();
        }
        client=client_bak;
        client.setTimeout(50);
    }

    if(client) {
        if(client.connected()) {
            return client.available();
        }
        client.stop();
    }
    return false;
}

int TcpDeviceConfig::read(void)
{
    return client.read();
}

String TcpDeviceConfig::readString(void)
{
    return client.readString();
}

size_t TcpDeviceConfig::write(const uint8_t *buf, size_t size)
{
    return client.write(buf,size);
}

void TcpDeviceConfig::close(void)
{
    client.stop();
}
#endif

#ifdef configSETUP_UDP_ENABLE
static volatile uint8_t UdpStep = 0;

void UdpDeviceConfig::init(void)
{
    Udp.setTimeout(50);
    network_setup(0, 0, NULL);
    wlan_Imlink_start();
    UdpStep = 0;
}

void UdpDeviceConfig::sendComfirm(int status)
{
    aJsonObject* root = aJson.createObject();
    if (root == NULL) {
      return;
    }

    aJson.addNumberToObject(root, "status", status);
    char* string = aJson.print(root);
    if(NULL != string) {
        for(int i=0; i < 10; i++) {
            write((unsigned char *)string, strlen(string));
            delay(100);
        }
        free(string);
    }
    aJson.deleteItem(root);
}

int UdpDeviceConfig::available(void)
{
    switch(UdpStep)
    {
        case 0:
            if( IMLINK_SUCCESS == wlan_Imlink_get_status() ) {
                wlan_Imlink_stop();
                network_connect(0, 0, 0, NULL);
                ARM_CONFIG_TIMEOUT(5000);
                UdpStep = 1;
            }
            break;
        case 1:
            if(IS_CONFIG_TIMEOUT()) {
                wlan_Imlink_start();
                CLR_CONFIG_TIMEOUT();
                UdpStep = 0;
            }
            if( network_ready(0, 0, NULL) ) {
                system_rgb_blink(RGB_COLOR_RED, 200);
                Udp.begin(5556);
                //Udp.beginPacket 放在此处是因为设备接收到udp数据后，可以自动获取remoteip和remoteport。
                //发送数据的时候不需要也不能指定remoteip和remoteport.
                Udp.beginPacket(IPADDR_BROADCAST,5557);
                ARM_CONFIG_TIMEOUT(10000);
                UdpStep = 2;
            }
            break;
        case 2:
            if(IS_CONFIG_TIMEOUT()) {
                Udp.stop();
                system_rgb_blink(RGB_COLOR_RED, 1000);
                wlan_Imlink_start();
                CLR_CONFIG_TIMEOUT();
                UdpStep = 0;
            }
            return Udp.parsePacket();
            break;
        default:
            break;
    }
    return 0;
}

int UdpDeviceConfig::read(void)
{
    return Udp.read();
}

String UdpDeviceConfig::readString(void)
{
    return Udp.readString();
}

size_t UdpDeviceConfig::write(const uint8_t *buf, size_t size)
{
    //需要清除缓存，因为接受和发送放在同一个缓存中。
    Udp.flush();
    Udp.write(buf,size);
    return Udp.endPacket();
}

void UdpDeviceConfig::close(void)
{
    Udp.stop();
    wlan_Imlink_stop();
}
#endif

#ifdef configSETUP_USBSERIAL_ENABLE
UsbDeviceConfig DeviceSetupUsbSerial;
#endif
#ifdef configSETUP_USARTSERIAL_ENABLE
UsartDeviceConfig DeviceSetupUsartSerial;
#endif
#ifdef configSETUP_TCP_ENABLE
TcpDeviceConfig DeviceSetupAp;
#endif
#ifdef configSETUP_UDP_ENABLE
UdpDeviceConfig DeviceSetupImlink;
#endif

static system_config_type_t current_system_config_type = SYSTEM_CONFIG_TYPE_NONE;       //配置类型
static uint8_t system_config_initial_flag = 0;     //是否已经执行配置初始化代码
static uint8_t system_config_finish_flag = 0;      //是否已经执行配置退出代码
volatile uint8_t g_intorobot_system_config = 0;    //是否在配置模式

void set_system_config_type(system_config_type_t config_type)
{
    CONFIG_FLAG_TypeDef flag = CONFIG_FLAG_NONE;

    if(current_system_config_type == config_type) {
        return;
    } else {
        current_system_config_type = config_type;
    }

    switch(config_type) {
        case SYSTEM_CONFIG_TYPE_IMLINK_SERIAL:   //进入串口配置模式
            flag = CONFIG_FLAG_IMLINK_SERIAL;
            break;
        case SYSTEM_CONFIG_TYPE_AP_SERIAL:      //进入ap+串口配置模式
            flag = CONFIG_FLAG_AP_SERIAL;
            break;
        case SYSTEM_CONFIG_TYPE_SERIAL:         //串口配置模式
            flag = CONFIG_FLAG_SERIAL;
            break;
        case SYSTEM_CONFIG_TYPE_IMLINK:         //进入imlink配置模式
            flag = CONFIG_FLAG_IMLINK;
            break;
        case SYSTEM_CONFIG_TYPE_AP:             //进入ap配置模式
            flag = CONFIG_FLAG_AP;
            break;
        default:   //退出配置模式
            current_system_config_type = SYSTEM_CONFIG_TYPE_NONE;
            flag = CONFIG_FLAG_NONE;
            break;
    }

    if(current_system_config_type == SYSTEM_CONFIG_TYPE_NONE) { //退出配置模式
        g_intorobot_system_config = 0;
    } else { //进入配置模式
        g_intorobot_system_config = 1;
        system_config_initial_flag = 0;
        system_config_finish_flag = 0;
    }

    //保存配置模式
    if(System.featureEnabled(SYSTEM_FEATURE_CONFIG_SAVE_ENABLED)) {
        if(flag != HAL_PARAMS_Get_System_config_flag()) {
            HAL_PARAMS_Set_System_config_flag(flag);
            HAL_PARAMS_Save_Params();
        }
    }
}

system_config_type_t get_system_config_type(void)
{
    return current_system_config_type;
}

void system_config_initial(void)
{
    SCONFIG_DEBUG("system config initial\r\n");
    RGB.control(false);  //进入配置模式由系统接管
    system_rgb_blink(RGB_COLOR_RED, 1000);
    HAL_Core_Enter_Config();
    switch(get_system_config_type()) {
        case SYSTEM_CONFIG_TYPE_IMLINK_SERIAL:   //进入串口配置模式
#ifdef configSETUP_UDP_ENABLE
            DeviceSetupImlink.init();
#endif
#ifdef configSETUP_USBSERIAL_ENABLE
            DeviceSetupUsbSerial.init();
#endif
#ifdef configSETUP_USARTSERIAL_ENABLE
            DeviceSetupUsartSerial.init();
#endif
            system_notify_event(event_mode_changed, ep_mode_imlink_serial_config);
            break;
        case SYSTEM_CONFIG_TYPE_AP_SERIAL:      //进入ap+串口配置模式
#ifdef configSETUP_TCP_ENABLE
            DeviceSetupAp.init();
#endif
#ifdef configSETUP_USBSERIAL_ENABLE
            DeviceSetupUsbSerial.init();
#endif
#ifdef configSETUP_USARTSERIAL_ENABLE
            DeviceSetupUsartSerial.init();
#endif
            system_notify_event(event_mode_changed, ep_mode_ap_serial_config);
            break;
        case SYSTEM_CONFIG_TYPE_SERIAL:         //串口配置模式
#ifdef configSETUP_USBSERIAL_ENABLE
            DeviceSetupUsbSerial.init();
#endif
#ifdef configSETUP_USARTSERIAL_ENABLE
            DeviceSetupUsartSerial.init();
#endif
            system_notify_event(event_mode_changed, ep_mode_serial_config);
            break;
        case SYSTEM_CONFIG_TYPE_IMLINK:         //进入imlink配置模式
#ifdef configSETUP_UDP_ENABLE
            DeviceSetupImlink.init();
#endif
            system_notify_event(event_mode_changed, ep_mode_imlink_config);
            break;
        case SYSTEM_CONFIG_TYPE_AP:             //进入ap配置模式
#ifdef configSETUP_TCP_ENABLE
            DeviceSetupAp.init();
#endif
            system_notify_event(event_mode_changed, ep_mode_ap_config);
            break;
        default:
            break;
    }
}

void system_config_finish(void)
{
    SCONFIG_DEBUG("system config finish\r\n");
#ifdef configSETUP_UDP_ENABLE
    DeviceSetupImlink.close();
#endif
#ifdef configSETUP_TCP_ENABLE
    DeviceSetupAp.close();
#endif
#ifdef configSETUP_USBSERIAL_ENABLE
    DeviceSetupUsbSerial.close();
#endif
#ifdef configSETUP_USARTSERIAL_ENABLE
    DeviceSetupUsartSerial.close();
#endif
    HAL_Core_Exit_Config();
    system_notify_event(event_mode_changed, ep_mode_normal);
    NEWORK_FN(Network_Setup(), (void)0);
    LORAWAN_FN(LoraWAN_Setup(), (void)0);
}

bool system_config_process(void)
{
    //执行配置初始化
    if(0 == system_config_initial_flag) {
        system_config_initial_flag = 1;
        CLOUD_FN(cloud_disconnect(false), (void)0);
        system_config_initial();
    }

    //配置处理
    switch(get_system_config_type()) {
        case SYSTEM_CONFIG_TYPE_IMLINK_SERIAL:   //进入imlink+串口配置模式
#ifdef configSETUP_UDP_ENABLE
            if(DeviceSetupImlink.process()) {
                goto success;
            }
#endif
#ifdef configSETUP_USBSERIAL_ENABLE
            if(DeviceSetupUsbSerial.process()) {
                goto success;
            }
#endif
#ifdef configSETUP_USARTSERIAL_ENABLE
            if(DeviceSetupUsartSerial.process()) {
                goto success;
            }
#endif
            break;
        case SYSTEM_CONFIG_TYPE_AP_SERIAL:      //进入ap+串口配置模式
#ifdef configSETUP_TCP_ENABLE
            if(DeviceSetupAp.process()) {
                goto success;
            }
#endif
#ifdef configSETUP_USBSERIAL_ENABLE
            if(DeviceSetupUsbSerial.process()) {
                goto success;
            }
#endif
#ifdef configSETUP_USARTSERIAL_ENABLE
            if(DeviceSetupUsartSerial.process()) {
                goto success;
            }
#endif
            break;
        case SYSTEM_CONFIG_TYPE_SERIAL:         //串口配置模式
#ifdef configSETUP_USBSERIAL_ENABLE
            if(DeviceSetupUsbSerial.process()) {
                goto success;
            }
#endif
#ifdef configSETUP_USARTSERIAL_ENABLE
            if(DeviceSetupUsartSerial.process()) {
                goto success;
            }
#endif
            break;
        case SYSTEM_CONFIG_TYPE_IMLINK:         //进入imlink配置模式
#ifdef configSETUP_UDP_ENABLE
            if(DeviceSetupImlink.process()) {
                goto success;
            }
#endif
            break;
        case SYSTEM_CONFIG_TYPE_AP:             //进入ap配置模式
#ifdef configSETUP_TCP_ENABLE
            if(DeviceSetupAp.process()) {
                goto success;
            }
#endif
            break;
        default:
            goto success;
            break;
    }
    return false;

success:
    //执行配置成功
    if(0 == system_config_finish_flag) {
        system_config_finish_flag = 1;
        system_config_finish();
        set_system_config_type(SYSTEM_CONFIG_TYPE_NONE);
    }
    return true;
}

// These are internal methods
void system_config_setup(void)
{
    if(System.featureEnabled(SYSTEM_FEATURE_CONFIG_SAVE_ENABLED)) {
        switch(HAL_PARAMS_Get_System_config_flag()) {
            case CONFIG_FLAG_IMLINK_SERIAL:   //进入imlink+串口配置模式
                set_system_config_type(SYSTEM_CONFIG_TYPE_IMLINK_SERIAL);
                break;
            case CONFIG_FLAG_AP_SERIAL:      //进入ap+串口配置模式
                set_system_config_type(SYSTEM_CONFIG_TYPE_AP_SERIAL);
                break;
            case CONFIG_FLAG_SERIAL:         //串口配置模式
                set_system_config_type(SYSTEM_CONFIG_TYPE_SERIAL);
                break;
            case CONFIG_FLAG_IMLINK:         //进入imlink配置模式
                set_system_config_type(SYSTEM_CONFIG_TYPE_IMLINK);
                break;
            case CONFIG_FLAG_AP:             //进入ap配置模式
                set_system_config_type(SYSTEM_CONFIG_TYPE_AP);
                break;
            default:
                set_system_config_type(SYSTEM_CONFIG_TYPE_NONE);
                break;
        }
    } else {
        set_system_config_type(SYSTEM_CONFIG_TYPE_NONE);
    }
}

void manage_system_config(void)
{
    if(System.featureEnabled(SYSTEM_FEATURE_AUTO_CONFIG_PROCESS_ENABLED)) {
        if(SYSTEM_CONFIG_TYPE_NONE != get_system_config_type()) {
            while(1) {
                if(system_config_process()) {
                    break;
                }
                HAL_Core_System_Yield();
            }
        }
    }
}

#endif
