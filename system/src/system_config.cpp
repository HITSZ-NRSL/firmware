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
#include "string_convert.h"
#include "system_mode.h"
#include "system_task.h"
#include "system_test.h"
#include "system_version.h"

/*debug switch*/
#define SYSTEM_CONFIG_DEBUG

#ifdef SYSTEM_CONFIG_DEBUG
#define SCONFIG_DEBUG(...)  do {DEBUG(__VA_ARGS__);}while(0)
#define SCONFIG_DEBUG_D(...)  do {DEBUG_D(__VA_ARGS__);}while(0)
#else
#define SCONFIG_DEBUG(...)
#define SCONFIG_DEBUG_D(...)
#endif

#ifndef configNO_NETWORK
using namespace intorobot;
#endif

typedef size_t (*writeHandler)(const uint8_t *buf, size_t size);

static volatile uint32_t config_timeout_start;
static volatile uint32_t config_timeout_duration;

inline void ARM_CONFIG_TIMEOUT(uint32_t dur) {
    config_timeout_start = HAL_Timer_Get_Milli_Seconds();
    config_timeout_duration = dur;
    SCONFIG_DEBUG("CONFIG WD Set %d",(dur));
}
inline bool IS_CONFIG_TIMEOUT() {
    return config_timeout_duration && ((HAL_Timer_Get_Milli_Seconds()-config_timeout_start)>config_timeout_duration);
}

inline void CLR_CONFIG_TIMEOUT() {
    config_timeout_duration = 0;
    SCONFIG_DEBUG("CONFIG WD Cleared, was %d", config_timeout_duration);
}

DeviceConfigCmdType DeviceConfig::getMessageType(char *s) {
    if(!strcmp(s,"hello")) {
        return DEVICE_CONFIG_HELLO;
    }
    else if(!strcmp(s,"checkWifi")) {
        return DEVICE_CONFIG_CHECK_WIFI;
    }
    else if(!strcmp(s,"getNetworkStatus")) {
        return DEVICE_CONFIG_GET_NETWORK_STATUS;
    }
    else if(!strcmp(s,"getWifiList")) {
        return DEVICE_CONFIG_GET_WIFI_LIST;
    }
    else if(!strcmp(s,"getInfo")) {
        return DEVICE_CONFIG_GET_INFO;
    }
    else if(!strcmp(s,"sendWifiInfo")) {
        return DEVICE_CONFIG_SEND_WIFI_INFO;
    }
    else if(!strcmp(s,"setNetworkCredentials")) {
        return DEVICE_CONFIG_SET_NETWORK_CREDENTIALS;
    }
    else if(!strcmp(s,"sendDeviceInfo")) {
        return DEVICE_CONFIG_SET_DEVICE_INFO;
    }
    else if(!strcmp(s,"setSecurity")) {
        return DEVICE_CONFIG_SET_SECURITY;
    }
    else if(!strcmp(s,"setInfo")) {
        return DEVICE_CONFIG_SET_INFO;
    }
    else if(!strcmp(s,"restartNetwork")) {
        return DEVICE_CONFIG_RESTART_NETWORK;
    }
    else if(!strcmp(s,"reset")) {
        return DEVICE_CONFIG_RESET;
    }
    else if(!strcmp(s,"reboot")) {
        return DEVICE_CONFIG_REBOOT;
    }
    else if(!strcmp(s,"exit")) {
        return DEVICE_CONFIG_EXIT;
    }
    else if(!strcmp(s,"test")) {
        return DEVICE_CONFIG_TEST;
    }
    else {
        return DEVICE_CONFIG_ERROR;
    }
}

int DeviceConfig::process(void)
{
    aJsonObject *root=NULL;
    aJsonObject* value_Object;
    bool _isConfigSuccessful=false;

    while(available())
    {
        String tmp=readString();

        SCONFIG_DEBUG("OK! Rev: %s", (char *)tmp.c_str());
        root = aJson.parse((char *)tmp.c_str());
        if (root == NULL)
        {break;}
        aJsonObject* command_Object = aJson.getObjectItem(root, "command");

        if (command_Object == NULL)
        {break;}

        DeviceConfigCmdType type = getMessageType(command_Object->valuestring);
        switch(type)
        {
            //查询类指令
            case DEVICE_CONFIG_HELLO:                   //获取设备基础信息
                dealHello();
                break;
            case DEVICE_CONFIG_CHECK_WIFI:              //获取wifi状态
                dealCheckWifi();
                break;
            case DEVICE_CONFIG_GET_WIFI_LIST:           //获取wifi列表
                dealGetWifiList();
                break;
            case DEVICE_CONFIG_GET_NETWORK_STATUS:      //查询网络状态
                dealGetNetworkStatus();
                break;
            case DEVICE_CONFIG_GET_INFO:                //获取设备信息
                dealGetInfo();
                break;
            //设置类指令
            case DEVICE_CONFIG_SEND_WIFI_INFO:          //设置wifi信息
            case DEVICE_CONFIG_SET_NETWORK_CREDENTIALS: //设置网络接入凭证
                value_Object = aJson.getObjectItem(root, "value");
                if (value_Object == NULL)
                {break;}
                dealSetNetworkCredentials(value_Object);
                break;
            case DEVICE_CONFIG_SET_DEVICE_INFO:         //设置设备信息
                value_Object = aJson.getObjectItem(root, "value");
                if (value_Object == NULL)
                {break;}
                dealSendDeviceInfo(value_Object);
                sendComfirm(200);
                _isConfigSuccessful=true;
                close();
                break;
            case DEVICE_CONFIG_SET_SECURITY:            //设置设备安全信息
                value_Object = aJson.getObjectItem(root, "value");
                if (value_Object == NULL)
                {break;}
                dealSetSecurity(value_Object);
                break;
            case DEVICE_CONFIG_SET_INFO:                //设置设备信息
                value_Object = aJson.getObjectItem(root, "value");
                if (value_Object == NULL)
                {break;}
                dealSetInfo(value_Object);
                break;
            //执行类指令
            case DEVICE_CONFIG_RESTART_NETWORK:         //重启网络
                dealRestartNetwork();
                break;
            case DEVICE_CONFIG_RESET:                   //设备恢复出厂设置
                dealReset();
                break;
            case DEVICE_CONFIG_REBOOT:                  //设备重启
                dealReboot();
                break;
            case DEVICE_CONFIG_EXIT:                    //退出配置模式
                sendComfirm(200);
                _isConfigSuccessful=true;
                close();
                break;
            //测试类指令
            case DEVICE_CONFIG_TEST:                    //设备测试
                value_Object = aJson.getObjectItem(root, "value");
                if (value_Object == NULL)
                {break;}
                dealTest(value_Object);
                break;
            case DEVICE_CONFIG_ERROR: //错误
                sendComfirm(201);
                break;
            default:
                break;
        }
        if(_isConfigSuccessful)
        {break;}
    }
    if(root!=NULL) {
        aJson.deleteItem(root);
    }

    if(_isConfigSuccessful) {
        return 0;
    }
    else
    {return 1;}
}

void DeviceConfig::sendComfirm(int status)
{
    aJsonObject* root = aJson.createObject();
    if (root == NULL)
    {return;}

    aJson.addNumberToObject(root, "status", status);
    char* string = aJson.print(root);
    write((unsigned char *)string, strlen(string));
    free(string);
    aJson.deleteItem(root);
}

void DeviceConfig::dealHello(void)
{
    aJsonObject* root = aJson.createObject();
    if (root == NULL)
    {return;}

    aJson.addNumberToObject(root, "status", 200);
    aJson.addNumberToObject(root, "version", 2);  //v2版本配置协议
    char device_id[32]="", board[32]="";
    system_platform_id(board);
    aJson.addStringToObject(root, "board", (char *)board);
    if (AT_MODE_FLAG_NONE != HAL_PARAMS_Get_System_at_mode()) {
        HAL_PARAMS_Get_System_device_id(device_id, sizeof(device_id));
        aJson.addStringToObject(root, "device_id", (char *)device_id);
    }
    aJson.addNumberToObject(root, "at_mode", HAL_PARAMS_Get_System_at_mode());
    char* string = aJson.print(root);
    write((unsigned char *)string, strlen(string));
    free(string);
    aJson.deleteItem(root);
}

void DeviceConfig::dealCheckWifi(void)
{
#ifdef configWIRING_WIFI_ENABLE
    aJsonObject* root = aJson.createObject();
    if (root == NULL)
    {return;}

    if(WiFi.ready()) {
        manage_ip_config();
        aJson.addNumberToObject(root, "status", 200);
        aJson.addStringToObject(root, "ssid", WiFi.SSID());
        aJson.addNumberToObject(root, "rssi", WiFi.RSSI());
    } else {
        aJson.addNumberToObject(root, "status", 201);
    }
    char* string = aJson.print(root);
    write((unsigned char *)string, strlen(string));
    free(string);
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
        if (root == NULL)
        {return;}
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
        write((unsigned char *)string, strlen(string));
        free(string);
        aJson.deleteItem(root);
    }
#elif (defined configWIRING_CELLULAR_ENABLE) || (defined configWIRING_LORA_ENABLE)
    sendComfirm(201);
#endif
}

void DeviceConfig::dealGetInfo(void)
{
    aJsonObject* root = aJson.createObject();
    if (root == NULL)
    {return;}

    aJson.addNumberToObject(root, "status", 200);

    aJsonObject* value_object = aJson.createObject();
    if (value_object == NULL)
    {return;}

    char device_id[32]="",board[32]="";
    system_platform_id(board);
    aJson.addStringToObject(value_object, "board", board);
    HAL_PARAMS_Get_System_device_id(device_id, sizeof(device_id));
    aJson.addStringToObject(value_object, "device_id", device_id);
    aJson.addNumberToObject(value_object, "at_mode", HAL_PARAMS_Get_System_at_mode());

    aJson.addNumberToObject(value_object, "zone", HAL_PARAMS_Get_System_zone());
#ifdef configWIRING_WIFI_ENABLE
    char domain[50] = {0};
    HAL_PARAMS_Get_System_sv_domain(domain, sizeof(domain));
    if (strlen(domain)) {
        aJson.addStringToObject(value_object, "sv_domain", domain);
    } else {
        aJson.addStringToObject(value_object, "sv_domain", INTOROBOT_SERVER_DOMAIN);
    }

    if (HAL_PARAMS_Get_System_sv_port() > 0) {
        aJson.addNumberToObject(value_object, "sv_port", HAL_PARAMS_Get_System_sv_port());
    } else {
        aJson.addNumberToObject(value_object, "sv_port", INTOROBOT_SERVER_PORT);
    }

    memset(domain, 0, sizeof(domain));
    HAL_PARAMS_Get_System_http_domain(domain, sizeof(domain));
    if (strlen(domain)) {
        aJson.addStringToObject(value_object, "http_domain", domain);
    } else {
        aJson.addStringToObject(value_object, "http_domain", INTOROBOT_HTTP_DOMAIN);
    }

    if (HAL_PARAMS_Get_System_http_port() > 0) {
        aJson.addNumberToObject(value_object, "http_port", HAL_PARAMS_Get_System_http_port());
    } else {
        aJson.addNumberToObject(value_object, "http_port", INTOROBOT_HTTP_PORT);
    }

    memset(domain, 0, sizeof(domain));
    HAL_PARAMS_Get_System_dw_domain(domain, sizeof(domain));
    if (strlen(domain)) {
        aJson.addStringToObject(value_object, "dw_domain", domain);
    } else {
        aJson.addStringToObject(value_object, "dw_domain", INTOROBOT_UPDATE_DOMAIN);
    }

    uint8_t stamac[6] = {0}, apmac[6] = {0};
    char macStr[20] = {0};

    wlan_get_macaddr(stamac, apmac);
    memset(macStr, 0, sizeof(macStr));
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", stamac[0], stamac[1], stamac[2], stamac[3], stamac[4], stamac[5]);
    aJson.addStringToObject(value_object, "stamac", macStr);
    memset(macStr, 0, sizeof(macStr));
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", apmac[0], apmac[1], apmac[2], apmac[3], apmac[4], apmac[5]);
    aJson.addStringToObject(value_object, "apmac", macStr);
#elif defined configWIRING_CELLULAR_ENABLE
#elif defined configWIRING_LORA_ENABLE
    char devaddr[12];
    HAL_PARAMS_Get_System_devaddr(devaddr, sizeof(devaddr));
    aJson.addStringToObject(value_object, "devaddr", devaddr);
#endif
    aJson.addItemToObject(root, "value", value_object);
    char* string = aJson.print(root);
    write((unsigned char *)string, strlen(string));
    free(string);
    aJson.deleteItem(root);
}

void DeviceConfig::dealSendWifiInfo(aJsonObject* value_Object)
{
#ifdef configWIRING_WIFI_ENABLE
    wlan_Imlink_stop();
    aJsonObject* ssidObject = aJson.getObjectItem(value_Object, "ssid");
    aJsonObject* passwdObject = aJson.getObjectItem(value_Object, "passwd");
    if ((ssidObject != NULL) && (passwdObject != NULL)) {
        if(0==strcmp(ssidObject->valuestring,"")) { //密码为空
            WiFi.setCredentials(ssidObject->valuestring);
        } else {
            WiFi.setCredentials(ssidObject->valuestring, passwdObject->valuestring);
        }
        network_setup(0, 0, NULL);
        network_connect(0, 0, 0, NULL);
        sendComfirm(200);
        return;
    }
    sendComfirm(201);
#elif defined configWIRING_CELLULAR_ENABLE
    sendComfirm(201);
#elif defined configWIRING_LORA_ENABLE
    sendComfirm(201);
#endif
}

void DeviceConfig::dealSetNetworkCredentials(aJsonObject* value_Object)
{
#ifdef configWIRING_WIFI_ENABLE
    dealSendWifiInfo(value_Object);
#elif defined configWIRING_CELLULAR_ENABLE
    sendComfirm(201);
#elif defined configWIRING_LORA_ENABLE
    sendComfirm(201);
#endif
}

void DeviceConfig::dealSendDeviceInfo(aJsonObject* value_Object)
{
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
    sendComfirm(200);
#elif (defined configWIRING_CELLULAR_ENABLE) || (defined configWIRING_LORA_ENABLE)
    sendComfirm(200);
#endif
}

void DeviceConfig::dealSetSecurity(aJsonObject* value_Object)
{
    bool flag = false;

    aJsonObject *deviceIdObject = aJson.getObjectItem(value_Object, "device_id");
    //at_mode
    aJsonObject *atModeObject = aJson.getObjectItem(value_Object, "at_mode");
    if (atModeObject != NULL) {
#if (defined configWIRING_WIFI_ENABLE) || (defined configWIRING_CELLULAR_ENABLE)
        aJsonObject *activationCodeObject = aJson.getObjectItem(value_Object, "activation_code");
        aJsonObject *accessTokenObject = aJson.getObjectItem(value_Object, "access_token");
        if(AT_MODE_FLAG_ABP == atModeObject->valueint) {
            if (deviceIdObject != NULL && accessTokenObject != NULL) {
                HAL_PARAMS_Set_System_device_id(deviceIdObject->valuestring);
                HAL_PARAMS_Set_System_access_token(accessTokenObject->valuestring);
                flag = true;
            }
        } else if(AT_MODE_FLAG_OTAA_INACTIVE == atModeObject->valueint) {
            if (deviceIdObject != NULL && activationCodeObject != NULL) {
                HAL_PARAMS_Set_System_device_id(deviceIdObject->valuestring);
                HAL_PARAMS_Set_System_activation_code(activationCodeObject->valuestring);
                flag = true;
            }
        } else if(AT_MODE_FLAG_OTAA_ACTIVE == atModeObject->valueint) {
            if (deviceIdObject != NULL && activationCodeObject != NULL && accessTokenObject != NULL) {
                HAL_PARAMS_Set_System_device_id(deviceIdObject->valuestring);
                HAL_PARAMS_Set_System_activation_code(activationCodeObject->valuestring);
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
        sendComfirm(200);
    } else {
        sendComfirm(201);
    }
}

void DeviceConfig::dealSetInfo(aJsonObject* value_object)
{
    bool flag = true;

    //zone
    aJsonObject* zoneObject = aJson.getObjectItem(value_object, "zone");
    if (zoneObject != NULL) {
        float valuefloat = (zoneObject->type==aJson_Int?(float)(zoneObject->valueint):zoneObject->valuefloat);
        if(valuefloat < -12 || valuefloat > 13)
        {valuefloat = 8.0;}
        HAL_PARAMS_Set_System_zone(valuefloat);
    }

#if (defined configWIRING_WIFI_ENABLE) || (defined configWIRING_CELLULAR_ENABLE)
    //mqtt server domain
    aJsonObject* svDomainObject = aJson.getObjectItem(value_object, "sv_domain");
    if (svDomainObject != NULL) {
        HAL_PARAMS_Set_System_sv_domain(svDomainObject->valuestring);
    }
    //mqtt server port
    aJsonObject* svPortObject = aJson.getObjectItem(value_object, "sv_port");
    if (svPortObject != NULL) {
        HAL_PARAMS_Set_System_sv_port(svPortObject->valueint);
    }
    //down server domain
    aJsonObject* dwDomainObject = aJson.getObjectItem(value_object, "dw_domain");
    if (dwDomainObject != NULL) {
        HAL_PARAMS_Set_System_dw_domain(dwDomainObject->valuestring);
    }
    //http server domain
    aJsonObject* httpDomainObject = aJson.getObjectItem(value_object, "http_domain");
    if (httpDomainObject != NULL) {
        HAL_PARAMS_Set_System_http_domain(httpDomainObject->valuestring);
    }
    //http server port
    aJsonObject* httpPortObject = aJson.getObjectItem(value_object, "http_port");
    if (httpPortObject != NULL) {
        HAL_PARAMS_Set_System_http_port(httpPortObject->valueint);
    }
#ifdef configWIRING_WIFI_ENABLE
    uint8_t stamac[6] = {0}, apmac[6] = {0};
    aJsonObject* stamacObject = aJson.getObjectItem(value_object, "stamac");
    aJsonObject* apmacObject = aJson.getObjectItem(value_object, "apmac");
    if ((stamacObject != NULL)&&(apmacObject != NULL)) {
        mac_str_to_bin(stamacObject->valuestring, stamac);
        mac_str_to_bin(apmacObject->valuestring, apmac);
        int ret = wlan_set_macaddr(stamac, apmac);
        if(wlan_set_macaddr(stamac, apmac)) {
            flag = false;
        }
    }
#endif
#endif
    if(true == flag) {
        HAL_PARAMS_Save_Params();
        sendComfirm(200);
    } else {
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

void DeviceConfig::dealTest(aJsonObject* value_object)
{
    uint16_t pinNum;
    uint8_t pinLevel;

    if(value_object == NULL) {
        return;
    }

    aJsonObject* itemObject = aJson.getObjectItem(value_object, "item");
    if(itemObject == NULL) {
        return ;
    }

    if(strcmp(itemObject->valuestring,"digitalWrite") == 0) {
        aJsonObject* pinObject = aJson.getObjectItem(value_object,"pin");
        if(pinObject == NULL) {
            return;
        }
        pinNum = pinObject->valueint;
        aJsonObject* valObject = aJson.getObjectItem(value_object,"val");
        if(valObject == NULL) {
            return;
        }

        if(strcmp(valObject->valuestring,"HIGH") == 0) {
            pinLevel = HIGH;
        } else {
            pinLevel = LOW;
        }
        testDigitalWrite(pinNum,pinLevel,this);
    } else if(strcmp(itemObject->valuestring,"analogRead") == 0) {
        aJsonObject* pinObject = aJson.getObjectItem(value_object,"pin");
        if(pinObject == NULL) {
            return;
        }
        pinNum = pinObject->valueint;
        testAnalogRead(pinNum,this);
    } else if(strcmp(itemObject->valuestring,"selfTest") == 0) {
        testSelfTest(this);
    } else if(strcmp(itemObject->valuestring,"rfCheck") == 0) {
        testRfCheck(this);
    } else if(strcmp(itemObject->valuestring,"sensorData") == 0) {
        testSensorData(this);
    }
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
    write((unsigned char *)string, strlen(string));
    free(string);
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
    write((unsigned char *)string, strlen(string));
    free(string);
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
    for(int i=0; i < 10; i++) //may be not enough
    {
        write((unsigned char *)string, strlen(string));
        delay(100);
    }
    free(string);
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
    if (root == NULL)
    {return;}

    aJson.addNumberToObject(root, "status", status);
    char* string = aJson.print(root);
    for(int i=0; i < 10; i++) //may be not enough
    {
        write((unsigned char *)string, strlen(string));
        delay(100);
    }
    free(string);
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
                UdpStep = 2;
            }
            break;
        case 2:
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
UsbDeviceConfig DeviceSetupSerial;
#endif
#ifdef configSETUP_USARTSERIAL_ENABLE
UsartDeviceConfig DeviceSetupSerial;
#endif
#ifdef configSETUP_TCP_ENABLE
TcpDeviceConfig DeviceSetupAp;
#endif
#ifdef configSETUP_UDP_ENABLE
UdpDeviceConfig DeviceSetupImlink;
#endif

static system_config_mode_t current_system_config_mode = SYSTEM_CONFIG_MODE_AUTOMATIC;  //配置模式处理方式
static system_config_type_t current_system_config_type = SYSTEM_CONFIG_TYPE_NONE;       //配置类型
static uint8_t system_config_initial_flag = 0;
volatile uint8_t g_intorobot_system_config = 0;    //默认不是配置状态

void set_system_config_mode(system_config_mode_t mode)
{
    current_system_config_mode = mode;
}

system_config_mode_t get_system_config_mode(void)
{
    return current_system_config_mode;
}

void set_system_config_type(system_config_type_t config_type)
{
    if(current_system_config_type == config_type) {
        return;
    }

    g_intorobot_system_config = 1;
    system_config_initial_flag = 0;
    current_system_config_type = config_type;
    switch(config_type)
    {
        case SYSTEM_CONFIG_TYPE_IMLINK_SERIAL:   //进入串口配置模式
            HAL_PARAMS_Set_System_config_flag(CONFIG_FLAG_IMLINK_SERIAL);
            break;
        case SYSTEM_CONFIG_TYPE_AP_SERIAL:      //进入ap+串口配置模式
            HAL_PARAMS_Set_System_config_flag(CONFIG_FLAG_AP_SERIAL);
            break;
        case SYSTEM_CONFIG_TYPE_SERIAL:         //串口配置模式
            HAL_PARAMS_Set_System_config_flag(CONFIG_FLAG_SERIAL);
            break;
        case SYSTEM_CONFIG_TYPE_IMLINK:         //进入imlink配置模式
            HAL_PARAMS_Set_System_config_flag(CONFIG_FLAG_IMLINK);
            break;
        case SYSTEM_CONFIG_TYPE_AP:             //进入ap配置模式
            HAL_PARAMS_Set_System_config_flag(CONFIG_FLAG_AP);
            break;
        default:   //退出配置模式
            g_intorobot_system_config = 0;
            HAL_PARAMS_Set_System_config_flag(CONFIG_FLAG_NONE);
            break;
    }
    HAL_PARAMS_Save_Params();
}

system_config_type_t get_system_config_type(void)
{
    return current_system_config_type;
}

void system_config_initial(void)
{
    switch(get_system_config_type())
    {
        case SYSTEM_CONFIG_TYPE_IMLINK_SERIAL:   //进入串口配置模式
#ifdef configSETUP_UDP_ENABLE
            DeviceSetupImlink.init();
#endif
            DeviceSetupSerial.init();
            break;
        case SYSTEM_CONFIG_TYPE_AP_SERIAL:      //进入ap+串口配置模式
#ifdef configSETUP_TCP_ENABLE
            DeviceSetupAp.init();
#endif
            DeviceSetupSerial.init();
            break;
        case SYSTEM_CONFIG_TYPE_SERIAL:         //串口配置模式
            DeviceSetupSerial.init();
            break;
        case SYSTEM_CONFIG_TYPE_IMLINK:         //进入imlink配置模式
#ifdef configSETUP_UDP_ENABLE
            DeviceSetupImlink.init();
#endif
            break;
        case SYSTEM_CONFIG_TYPE_AP:             //进入ap配置模式
#ifdef configSETUP_TCP_ENABLE
            DeviceSetupAp.init();
#endif
            break;
        default:
            break;
    }
}

void system_config_finish(void)
{
#ifdef configSETUP_UDP_ENABLE
    DeviceSetupImlink.close();
#endif
#ifdef configSETUP_TCP_ENABLE
    DeviceSetupAp.close();
#endif
    DeviceSetupSerial.close();
    NEWORK_FN(Network_Setup(), (void)0);
    LORAWAN_FN(LoraWAN_Setup(), (void)0);
}

int system_config_process(void)
{
    int result = 0;
    system_config_type_t config_type = get_system_config_type();
    static system_config_type_t system_config_type = SYSTEM_CONFIG_TYPE_NONE;

    if(SYSTEM_CONFIG_TYPE_NONE == config_type) {
        if(system_config_type != config_type) {
            system_config_finish();
        }
        system_config_type = config_type;
        return 0;
    }
    system_config_type = config_type;

    if(0 == system_config_initial_flag) {
        system_config_initial();
        system_config_initial_flag = 1;
    }

    switch(get_system_config_type())
    {
        case SYSTEM_CONFIG_TYPE_IMLINK_SERIAL:   //进入imlink+串口配置模式
#ifdef configSETUP_UDP_ENABLE
            result = DeviceSetupImlink.process();
            if(!result) {
                break;
            }
#endif
            result = DeviceSetupSerial.process();
            break;
        case SYSTEM_CONFIG_TYPE_AP_SERIAL:      //进入ap+串口配置模式
#ifdef configSETUP_TCP_ENABLE
            result = DeviceSetupAp.process();
            if(!result) {
                break;
            }
#endif
            result = DeviceSetupSerial.process();
            break;
        case SYSTEM_CONFIG_TYPE_SERIAL:         //串口配置模式
            result = DeviceSetupSerial.process();
            break;
        case SYSTEM_CONFIG_TYPE_IMLINK:         //进入imlink配置模式
#ifdef configSETUP_TCP_ENABLE
            result = DeviceSetupImlink.process();
#endif
            break;
        case SYSTEM_CONFIG_TYPE_AP:             //进入ap配置模式
#ifdef configSETUP_TCP_ENABLE
            result = DeviceSetupAp.process();
#endif
            break;
        default:
            break;
    }
    if(!result) {
        system_config_finish();
        set_system_config_type(SYSTEM_CONFIG_TYPE_NONE);
    }
    return result;
}

// These are internal methods
void system_config_setup(void)
{
    switch(HAL_PARAMS_Get_System_config_flag())
    {
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
}

void manage_system_config(void)
{
    if(SYSTEM_CONFIG_MODE_AUTOMATIC == get_system_config_mode()) { //配置处理流程放后台处理,阻塞处理。
        if(SYSTEM_CONFIG_TYPE_NONE != get_system_config_type()) {
            CLOUD_FN(intorobot_cloud_disconnect(), (void)0);
            system_rgb_blink(RGB_COLOR_RED, 1000);
            while(1)
            {
                if(0 == system_config_process())
                {break;}
                HAL_Core_System_Yield();
            }
        }
    }
}

#endif
