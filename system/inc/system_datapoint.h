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

#ifndef SYSTEM_DATAPOINT_H_
#define SYSTEM_DATAPOINT_H_

#include "intorobot_config.h"

#include <string.h>
#include <stdint.h>
#include "static_assert.h"
#include "wiring_string.h"

// Permission
#define JSON_DATA_FORMAT    0x30
#define BINARY_DATA_FORMAT  0x31

// Permission
#define UP_ONLY    "up_only"
#define DOWN_ONLY  "down_only"
#define UP_DOWN    "up_down"

// Polices
#define TIMED      "timed"
#define ON_CHANGE  "on_change"

#define PROPERTIES_MAX 50

typedef enum{
    DATA_TYPE_BOOL = 0,   //bool型
    DATA_TYPE_NUM,        //数值型
    DATA_TYPE_ENUM,       //枚举型
    DATA_TYPE_STRING,     //字符串型
    DATA_TYPE_BINARY      //透传型
}data_type_t;

typedef enum{
    RESULT_DATAPOINT_OLD  = 0,   // 旧数据
    RESULT_DATAPOINT_NEW  = 1,   // 新收取数据
    RESULT_DATAPOINT_NONE = 2,   // 没有该数据点
}read_datapoint_result_t;

//float型属性
struct float_property_t{
    double minValue;
    double maxValue;
    int resolution;
};

//透传型属性
struct binary_property_t{
    const uint8_t *value;
    uint16_t len;
};

// Property configuration
struct property_conf {
    const uint16_t dpID;
    const data_type_t dataType;
    const char *permission;
    const char *policy;
    long lapse;
    long runtime;
    read_datapoint_result_t readFlag;
    float_property_t floatProperty;
    String value;
    uint8_t *binaryValue;
    uint16_t binaryLen;
};

#ifdef __cplusplus
extern "C" {
#endif

void intorobotDefineDatapointBool(const uint16_t dpID, const char *permission, const bool value, const char *policy, const int lapse);
void intorobotDefineDatapointNumber(const uint16_t dpID, const char *permission, const double minValue, const double maxValue, const int resolution, const double value, const char *policy, const int lapse);
void intorobotDefineDatapointEnum(const uint16_t dpID, const char *permission, const int value, const char *policy, const int lapse);
void intorobotDefineDatapointString(const uint16_t dpID, const char *permission, const char *value, const char *policy, const int lapse);
void intorobotDefineDatapointBinary(const uint16_t dpID, const char *permission, const uint8_t *value, const uint16_t len, const char *policy, const int lapse);

read_datapoint_result_t intorobotReadDatapointBool(const uint16_t dpID, bool &value);
read_datapoint_result_t intorobotReadDatapointInt(const uint16_t dpID, int &value);
read_datapoint_result_t intorobotReadDatapointInt32(const uint16_t dpID, int32_t &value);
read_datapoint_result_t intorobotReadDatapointUint32(const uint16_t dpID, uint32_t &value);
read_datapoint_result_t intorobotReadDatapointDouble(const uint16_t dpID, double &value);
read_datapoint_result_t intorobotReadDatapointFloat(const uint16_t dpID, float &value);
read_datapoint_result_t intorobotReadDatapointString(const uint16_t dpID, String &value);
read_datapoint_result_t intorobotReadDatapointStringChar(const uint16_t dpID, char *value);
read_datapoint_result_t intorobotReadDatapointBinary(const uint16_t dpID, uint8_t *value, uint16_t &len);

void intorobotParseReceiveDataJson(uint8_t *payload, uint32_t len);
void intorobotParseReceiveDataBinary(uint8_t *payload, uint32_t len);
void intorobotWriteDatapoint(const uint16_t dpID, const char *value);
void intorobotSendDatapoint(const uint16_t dpID, const char *value);
void intorobotSendDatapointAll(void);

#ifdef __cplusplus
}
#endif

#endif	/* SYSTEM_CLOUDDATA_H_ */
