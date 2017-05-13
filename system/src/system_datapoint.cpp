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
#include <math.h>
#include "ajson.h"
#include "wiring.h"
#include "wiring_string.h"
#include "system_cloud_def.h"
#include "system_cloud.h"
#include "system_datapoint.h"
#include "system_lorawan.h"

/*debug switch*/
#define SYSTEM_DATAPOINT_DEBUG

#ifdef SYSTEM_DATAPOINT_DEBUG
#define SDATAPOINT_DEBUG(...)  do {DEBUG(__VA_ARGS__);}while(0)
#define SDATAPOINT_DEBUG_D(...)  do {DEBUG_D(__VA_ARGS__);}while(0)
#else
#define SDATAPOINT_DEBUG(...)
#define SDATAPOINT_DEBUG_D(...)
#endif

property_conf* properties[PROPERTIES_MAX];
int properties_count = 0;

int intorobotDiscoverProperty(const uint16_t dpID)
{
    for (int i = 0; i < properties_count; i++)
    {
        if (properties[i]->dpID == dpID) {
            return i;
        }
    }
    return -1;
}

void intorobotDefineDatapointBool(const uint16_t dpID, const char *permission, const bool value, const char *policy, const int lapse)
{
    if (-1 == intorobotDiscoverProperty(dpID)) {
        // Create property structure
        property_conf* prop = new property_conf {dpID, DATA_TYPE_BOOL, permission, policy, (long)lapse*1000, 0, RESULT_DATAPOINT_OLD};
        prop->value = String(value);
        properties[properties_count] = prop; // Save pointer to scructure
        properties_count++; // count the number of properties
    }
}

void intorobotDefineDatapointNumber(const uint16_t dpID, const char *permission, const double minValue, const double maxValue, const int resolution, const double value, const char *policy, const int lapse)
{
    if (-1 == intorobotDiscoverProperty(dpID)) {
        property_conf* prop;
        double defaultValue = value;
        // Create property structure
        prop = new property_conf {dpID, DATA_TYPE_NUM, permission, policy, (long)lapse*1000, 0, RESULT_DATAPOINT_OLD};
        prop->floatProperty.minValue = String(minValue, resolution).toDouble();
        prop->floatProperty.maxValue = String(maxValue, resolution).toDouble();
        prop->floatProperty.resolution = resolution;
        if(defaultValue < minValue) {
            defaultValue = minValue;
        } else if(defaultValue > maxValue) {
            defaultValue = maxValue;
        }
        prop->value = String(defaultValue, resolution);
        properties[properties_count] = prop; // Save pointer to scructure
        properties_count++; // count the number of properties
    }
}

void intorobotDefineDatapointEnum(const uint16_t dpID, const char *permission, const int value, const char *policy, const int lapse)
{
    if (-1 == intorobotDiscoverProperty(dpID)) {
        double defaultValue = value;
        // Create property structure
        property_conf* prop = new property_conf {dpID, DATA_TYPE_ENUM, permission, policy, (long)lapse*1000, 0, RESULT_DATAPOINT_OLD};
        if(defaultValue < 0) {
            defaultValue = 0;
        }
        prop->value = String(defaultValue);
        properties[properties_count] = prop; // Save pointer to scructure
        properties_count++; // count the number of properties
    }
}

void intorobotDefineDatapointString(const uint16_t dpID, const char *permission, const char *value, const char *policy, const int lapse)
{
    if (-1 == intorobotDiscoverProperty(dpID)) {
        // Create property structure
        property_conf* prop = new property_conf {dpID, DATA_TYPE_STRING, permission, policy, (long)lapse*1000, 0, RESULT_DATAPOINT_OLD};
        prop->value = value;
        properties[properties_count] = prop; // Save pointer to scructure
        properties_count++; // count the number of properties
    }
}

void intorobotDefineDatapointBinary(const uint16_t dpID, const char *permission, const uint8_t *value, const uint16_t len, const char *policy, const int lapse)
{
    if (-1 != intorobotDiscoverProperty(dpID)) {
        // Create property structure
        property_conf* prop = new property_conf {dpID, DATA_TYPE_BINARY, permission, policy, (long)lapse*1000, 0, RESULT_DATAPOINT_OLD};
        prop->binaryValue = (uint8_t *)value;
        prop->binaryLen = (uint16_t)len;
        properties[properties_count] = prop; // Save pointer to scructure
        properties_count++; // count the number of properties
    }
}

read_datapoint_result_t intorobotReadDatapointBool(const uint16_t dpID, bool &value)
{
    int index = intorobotDiscoverProperty(dpID);
    if (index == -1) {
        return RESULT_DATAPOINT_NONE;
    }

    value = (bool)(properties[index]->value.toInt());
    read_datapoint_result_t readResult = properties[index]->readFlag;
    properties[index]->readFlag = RESULT_DATAPOINT_OLD;
    return readResult;
}

read_datapoint_result_t intorobotReadDatapointInt(const uint16_t dpID, int &value)
{
    int index = intorobotDiscoverProperty(dpID);
    if (index == -1) {
        return RESULT_DATAPOINT_NONE;
    }

    value = (int)(properties[index]->value.toInt());
    read_datapoint_result_t readResult = properties[index]->readFlag;
    properties[index]->readFlag = RESULT_DATAPOINT_OLD;
    return readResult;
}

read_datapoint_result_t intorobotReadDatapointInt32(const uint16_t dpID, int32_t &value)
{
    int index = intorobotDiscoverProperty(dpID);
    if (index == -1) {
        return RESULT_DATAPOINT_NONE;
    }

    value = (int32_t)(properties[index]->value.toInt());
    read_datapoint_result_t readResult = properties[index]->readFlag;
    properties[index]->readFlag = RESULT_DATAPOINT_OLD;
    return readResult;
}

read_datapoint_result_t intorobotReadDatapointUint32(const uint16_t dpID, uint32_t &value)
{
    int index = intorobotDiscoverProperty(dpID);
    if (index == -1) {
        return RESULT_DATAPOINT_NONE;
    }

    value = (uint32_t)(properties[index]->value.toInt());
    read_datapoint_result_t readResult = properties[index]->readFlag;
    properties[index]->readFlag = RESULT_DATAPOINT_OLD;
    return readResult;
}

read_datapoint_result_t intorobotReadDatapointFloat(const uint16_t dpID, float &value)
{
    int index = intorobotDiscoverProperty(dpID);
    if (index == -1) {
        return RESULT_DATAPOINT_NONE;
    }

    value = properties[index]->value.toDouble();
    read_datapoint_result_t readResult = properties[index]->readFlag;
    properties[index]->readFlag = RESULT_DATAPOINT_OLD;
    return readResult;
}

read_datapoint_result_t intorobotReadDatapointDouble(const uint16_t dpID, double &value)
{
    int index = intorobotDiscoverProperty(dpID);
    if (index == -1) {
        return RESULT_DATAPOINT_NONE;
    }

    value = properties[index]->value.toDouble();
    read_datapoint_result_t readResult = properties[index]->readFlag;
    properties[index]->readFlag = RESULT_DATAPOINT_OLD;
    return readResult;
}

read_datapoint_result_t intorobotReadDatapointString(const uint16_t dpID, String &value)
{
    int index = intorobotDiscoverProperty(dpID);
    if (index == -1) {
        return RESULT_DATAPOINT_NONE;
    }

    value = properties[index]->value;
    read_datapoint_result_t readResult = properties[index]->readFlag;
    properties[index]->readFlag = RESULT_DATAPOINT_OLD;
    return readResult;
}

read_datapoint_result_t intorobotReadDatapointStringChar(const uint16_t dpID, char *value)
{
    int index = intorobotDiscoverProperty(dpID);
    if (index == -1) {
        return RESULT_DATAPOINT_NONE;
    }

    value = (char *)(properties[index]->value.c_str());
    read_datapoint_result_t readResult = properties[index]->readFlag;
    properties[index]->readFlag = RESULT_DATAPOINT_OLD;
    return readResult;
}

read_datapoint_result_t intorobotReadDatapointBinary(const uint16_t dpID, uint8_t *value, uint16_t &len)
{
    int index = intorobotDiscoverProperty(dpID);
    if (index == -1) {
        return RESULT_DATAPOINT_NONE;
    }

    value = properties[index]->binaryValue;
    len = properties[index]->binaryLen;
    read_datapoint_result_t readResult = properties[index]->readFlag;
    properties[index]->readFlag = RESULT_DATAPOINT_OLD;
    return readResult;
}

void intorobotWriteDatapoint(const uint16_t dpID, const char* value)
{
    int i = intorobotDiscoverProperty(dpID);

    if (i == -1) {
        // not found, nothing to do
        return;
    }

    if(DATA_TYPE_NUM == properties[i]->dataType) {
        //根据分辨率  截取小数点位数。
        if(String(value).toDouble() < properties[i]->floatProperty.minValue) {
            properties[i]->value = String(properties[i]->floatProperty.minValue, properties[i]->floatProperty.resolution);
        } else if(String(value).toDouble() > properties[i]->floatProperty.maxValue) {
            properties[i]->value = String(properties[i]->floatProperty.maxValue, properties[i]->floatProperty.resolution);
        } else {
            properties[i]->value = String(String(value).toDouble(), properties[i]->floatProperty.resolution);
        }
    } else {
        properties[i]->value = value;
    }
}

/*
 * json datapoint
 * */
#ifndef configNO_CLOUD
void intorobotParseReceiveDataJson(uint8_t *payload, uint32_t len)
{
    aJsonClass aJson;

    SDATAPOINT_DEBUG("OK! Rev datapoint data: %s", payload);
    if(payload[0] != JSON_DATA_FORMAT) {
        SDATAPOINT_DEBUG("Error! Not Json data format");
        return;
    }

    aJsonObject *root = aJson.parse((char *)(payload+1));
    if (root == NULL) {
        return;
    }

    for (int i = 0; i < properties_count; i++) {
        if (0 != strcmp(properties[i]->permission, DP_PERMISSION_UP_ONLY)) {
            aJsonObject* propertyObject = aJson.getObjectItem(root, String(properties[i]->dpID).c_str());
            if (propertyObject != NULL) {
                switch(properties[i]->dataType)
                {
                    case DATA_TYPE_BOOL:       //bool型
                        if(aJson_Boolean == propertyObject->type) {
                            properties[i]->value = String((int)propertyObject->valuebool);
                            properties[i]->readFlag = RESULT_DATAPOINT_NEW;
                        }
                        break;
                    case DATA_TYPE_NUM:        //数值型 int型
                        {
                            if(aJson_Int == propertyObject->type) {
                                double valueDouble = propertyObject->valueint/pow(10, properties[i]->floatProperty.resolution)\
                                                     + properties[i]->floatProperty.minValue;
                                properties[i]->value = String(valueDouble, properties[i]->floatProperty.resolution);
                                properties[i]->readFlag = RESULT_DATAPOINT_NEW;
                            }
                            break;
                        }
                    case DATA_TYPE_ENUM:       //枚举型
                        if(aJson_Int == propertyObject->type) {
                            properties[i]->value = String(propertyObject->valueint);
                            properties[i]->readFlag = RESULT_DATAPOINT_NEW;
                        }
                        break;
                    case DATA_TYPE_STRING:     //字符串型
                        if(aJson_String == propertyObject->type) {
                            properties[i]->value = String(propertyObject->valuestring);
                            properties[i]->readFlag = RESULT_DATAPOINT_NEW;
                        }
                        break;
                    default:
                        break;
                }
            }
        }
    }
    aJson.deleteItem(root);
}

static String intorobotFormSinglePropertyJson(int property_index)
{
    aJsonClass aJson;
    String PropertyJson((char)JSON_DATA_FORMAT);

    aJsonObject* root = aJson.createObject();
    switch(properties[property_index]->dataType)
    {
        case DATA_TYPE_BOOL:       //bool型
            aJson.addBooleanToObject(root, String(properties[property_index]->dpID).c_str(), (bool)(properties[property_index]->value.toInt()));
            break;
        case DATA_TYPE_NUM:        //数值型
            {
                int value = (properties[property_index]->value.toDouble() - properties[property_index]->floatProperty.minValue) * pow(10, properties[property_index]->floatProperty.resolution);
                aJson.addNumberToObject(root, String(properties[property_index]->dpID).c_str(), value);
                break;
            }
        case DATA_TYPE_ENUM:       //枚举型
            aJson.addNumberToObject(root, String(properties[property_index]->dpID).c_str(), (int)(properties[property_index]->value.toInt()));
            break;
        case DATA_TYPE_STRING:     //字符串型
            aJson.addStringToObject(root, String(properties[property_index]->dpID).c_str(), properties[property_index]->value.c_str());
            break;
        default:
            break;
    }
    char* strPtr = aJson.print(root);
    PropertyJson += strPtr;
    free(strPtr);
    aJson.deleteItem(root);

    return PropertyJson;
}

static String intorobotFormAllPropertyJson(void)
{
    aJsonClass aJson;
    String PropertyJson((char)JSON_DATA_FORMAT);

    aJsonObject* root = aJson.createObject();
    for (int i = 0; i < properties_count; i++)
    {
        //只允许下发  不上传
        if (0 == strcmp(properties[i]->permission, DP_PERMISSION_DOWN_ONLY)) {
            continue;
        }

        //系统默认dpID  不上传
        if (properties[i]->dpID > 0xFF00) {
            continue;
        }

        switch(properties[i]->dataType)
        {
            case DATA_TYPE_BOOL:       //bool型
                aJson.addBooleanToObject(root, String(properties[i]->dpID).c_str(), (bool)(properties[i]->value.toInt()));
                break;
            case DATA_TYPE_NUM:        //数值型
                {
                    int value = (properties[i]->value.toDouble() -  properties[i]->floatProperty.minValue)\
                                * pow(10, properties[i]->floatProperty.resolution);
                    aJson.addNumberToObject(root, String(properties[i]->dpID).c_str(), value);
                    break;
                }
            case DATA_TYPE_ENUM:       //枚举型
                aJson.addNumberToObject(root, String(properties[i]->dpID).c_str(), (int)(properties[i]->value.toInt()));
                break;
            case DATA_TYPE_STRING:     //字符串型
                aJson.addStringToObject(root, String(properties[i]->dpID).c_str(), properties[i]->value.c_str());
                break;
            default:
                break;
        }
    }

    char* strPtr = aJson.print(root);
    PropertyJson += strPtr;
    free(strPtr);
    aJson.deleteItem(root);

    return PropertyJson;
}

void intorobotSendDatapoint(const uint16_t dpID, const char* value)
{
    int i = intorobotDiscoverProperty(dpID);

    if (i == -1) {
        // not found, nothing to do
        return;
    }

    intorobotWriteDatapoint(dpID, value);

    //只允许下发
    if ( 0 == strcmp(properties[i]->permission, DP_PERMISSION_DOWN_ONLY) ) {
        SDATAPOINT_DEBUG("only permit cloud -> terminal %d", properties[i]->dpID);
        return;
    }

    //数值未发生变化
    if (properties[i]->value.equals(value) && strcmp(properties[i]->policy, DP_POLICY_ON_CHANGE) == 0) {
        SDATAPOINT_DEBUG("No Changes for %d:%d", properties[i]->dpID, value);
        return;
    }

    //发送时间间隔到
    system_tick_t current_millis = millis();
    system_tick_t elapsed_millis = current_millis - properties[i]->runtime;
    if (elapsed_millis < 0) {
        elapsed_millis =  0xFFFFFFFF - properties[i]->runtime + current_millis;
    }

    if (elapsed_millis >= properties[i]->lapse) {
        String payload = intorobotFormSinglePropertyJson(i);
        intorobot_publish(API_VERSION_V2, INTOROBOT_MQTT_RX_TOPIC, (uint8_t *)payload.c_str(), payload.length(), 0, false);
        properties[i]->runtime = current_millis;
    }
}

void intorobotSendDatapointAll(void)
{
    String payload = intorobotFormAllPropertyJson();
    intorobot_publish(API_VERSION_V2, INTOROBOT_MQTT_RX_TOPIC, (uint8_t *)payload.c_str(), payload.length(), 0, false);
}

#endif

/*
 * binary datapoint
 * */
#ifndef configNO_LORAWAN
void intorobotParseReceiveDataBinary(uint8_t *payload, uint32_t len)
{
    SDATAPOINT_DEBUG("OK! Rev datapoint data!");
    if(payload[0] != BINARY_DATA_FORMAT) {
        SDATAPOINT_DEBUG("Error! Not Binary data format");
        return;
    }
}

static uint16_t intorobotFormDataPointBinary(int property_index, uint8_t* buffer)
{
    int32_t index = 0;

    if(properties[property_index]->dpID < 0x80) {
        buffer[index++] = properties[property_index]->dpID & 0xFF;
    } else {
        buffer[index++] = (properties[property_index]->dpID >> 8) | 0x80;
        buffer[index++] = properties[property_index]->dpID & 0xFF;
    }
    switch(properties[property_index]->dataType)
    {
        case DATA_TYPE_BOOL:       //bool型
            buffer[index++] = 0x00;  //类型
            buffer[index++] = 0x01;  //长度
            buffer[index++] = (bool)(properties[property_index]->value.toInt());
            break;
        case DATA_TYPE_NUM:        //数值型 int型
            {
                buffer[index++] = 0x01;
                uint32_t value = (properties[property_index]->value.toDouble() - properties[property_index]->floatProperty.minValue) \
                                 * pow(10, properties[property_index]->floatProperty.resolution);
                if(value & 0xFFFF0000) {
                    buffer[index++] = 0x04;
                    buffer[index++] = (value >> 24) & 0xFF;
                    buffer[index++] = (value >> 16) & 0xFF;
                    buffer[index++] = (value >> 8) & 0xFF;
                    buffer[index++] = value & 0xFF;
                } else if(value & 0xFFFFFF00) {
                    buffer[index++] = 0x02;
                    buffer[index++] = (value >> 8) & 0xFF;
                    buffer[index++] = value & 0xFF;
                } else {
                    buffer[index++] = 0x01;
                    buffer[index++] = value & 0xFF;
                }
            }
            break;
        case DATA_TYPE_ENUM:       //枚举型
            buffer[index++] = 0x02;
            buffer[index++] = 0x01;
            buffer[index++] = (uint8_t)properties[property_index]->value.toInt() & 0xFF;
            break;
        case DATA_TYPE_STRING:     //字符串型
            {
                uint16_t strlen = properties[property_index]->value.length();

                buffer[index++] = 0x03;
                if(strlen < 0x80) {
                    buffer[index++] = strlen & 0xFF;
                } else {
                    buffer[index++] = (strlen >> 8) | 0x80;
                    buffer[index++] = strlen & 0xFF;
                }
                memcpy(&buffer[index], properties[property_index]->value.c_str(), strlen);
                index+=strlen;
                break;
            }
        default:
            break;
    }
    return index;
}

static uint16_t intorobotFormSinglePropertyBinary(int property_index, char* buffer, uint16_t len)
{
    int32_t index = 0;

    buffer[index++] = BINARY_DATA_FORMAT;
    index += intorobotFormDataPointBinary(property_index, buffer+index);
    return index;
}

static uint16_t intorobotFormAllPropertyBinary(char* buffer, uint16_t len)
{
    int32_t index = 0;

    buffer[index++] = BINARY_DATA_FORMAT;
    for (int i = 0; i < properties_count; i++)
    {
        //只允许下发  不上传
        if (0 == strcmp(properties[i]->permission, DP_PERMISSION_DOWN_ONLY)) {
            continue;
        }

        //系统默认dpID  不上传
        if (properties[i]->dpID > 0xFF00) {
            continue;
        }

        index += intorobotFormDataPointBinary(i, buffer+index);
    }
    return index;
}

//datepoint process
void intorobotSendDatapoint(const uint16_t dpID, const char* value)
{
    int i = intorobotDiscoverProperty(dpID);

    if (i == -1) {
        // not found, nothing to do
        return;
    }

    intorobotWriteDatapoint(dpID, value);

    //只允许下发
    if ( 0 == strcmp(properties[i]->permission, DP_PERMISSION_DOWN_ONLY) ) {
        SDATAPOINT_DEBUG("only permit cloud -> terminal %d", properties[i]->dpID);
        return;
    }

    //数值未发生变化
    if (properties[i]->value.equals(value) && strcmp(properties[i]->policy, DP_POLICY_ON_CHANGE) == 0) {
        SDATAPOINT_DEBUG("No Changes for %d:%d", properties[i]->dpID, value);
        return;
    }

    //发送时间间隔到
    system_tick_t current_millis = millis();
    system_tick_t elapsed_millis = current_millis - properties[i]->runtime;
    if (elapsed_millis < 0) {
        elapsed_millis =  0xFFFFFFFF - properties[i]->runtime + current_millis;
    }

    if (elapsed_millis >= properties[i]->lapse) {
        uint8_t buffer[256];
        uint16_t len;

        len = intorobotFormSinglePropertyBinary(dpID, buffer, sizeof(buffer));
        intorobot_lorawan_send_data(buffer, len);
        properties[i]->runtime = current_millis;
    }
}

void intorobotSendDatapointAll(void)
{
    uint8_t buffer[256];
    uint16_t len;

    len = intorobotFormAllPropertyBinary(buffer, sizeof(buffer));
    intorobot_lorawan_send_data(buffer, len);
}

#endif
