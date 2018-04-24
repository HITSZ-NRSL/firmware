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

// data protocol
#define DATA_PROTOCOL_DATAPOINT_JSON              0x30
#define DATA_PROTOCOL_DATAPOINT_BINARY            0x31
#define DATA_PROTOCOL_CUSTOM                      0x32

#define PROPERTIES_MAX                            50

#define DATAPOINT_TRANSMIT_AUTOMATIC_INTERVAL     600

#define DPID_DEFAULT_BOOL_RESET                   0x7F80        //默认数据点  复位
#define DPID_DEFAULT_BOOL_GETALLDATAPOINT         0x7F81        //默认数据点  获取所有数据点

// transmit
typedef enum {
    DP_TRANSMIT_MODE_MANUAL = 0,       // 用户控制发送
    DP_TRANSMIT_MODE_AUTOMATIC,        // 系统自动发送
}dp_transmit_mode_t;

// Permission
typedef enum {
    DP_PERMISSION_UP_ONLY = 0,   //只上送
    DP_PERMISSION_DOWN_ONLY,     //只下发
    DP_PERMISSION_UP_DOWN        //可上送下发
}dp_permission_t;

// Policy
typedef enum {
    DP_POLICY_NONE = 0,         //立即发送
    DP_POLICY_TIMED,            //间隔发送
    DP_POLICY_ON_CHANGE         //改变发送
}dp_policy_t;

typedef enum {
    DATA_TYPE_BOOL = 0,         //bool型
    DATA_TYPE_NUM = 1,          //数值型
    DATA_TYPE_ENUM = 2,         //枚举型
    DATA_TYPE_STRING = 3,       //字符串型
    DATA_TYPE_BINARY = 4        //透传型
}data_type_t;

typedef enum {
    RESULT_DATAPOINT_OLD = 0,   // 旧数据
    RESULT_DATAPOINT_NEW,       // 新收取数据
    RESULT_DATAPOINT_NONE       // 没有该数据点
}read_datapoint_result_t;

//number型属性
typedef struct {
    double minValue;
    double maxValue;
    int resolution;
}number_property_t;

//binary型属性
typedef struct {
    uint8_t *value;
    uint16_t len;
}binary_property_t;

//datapoint uint
typedef struct {
    const uint16_t dpID;
    const data_type_t dataType;
    const dp_permission_t permission;
    const dp_policy_t policy;
    uint32_t lapse;
    uint32_t runtime;
    bool change;
    read_datapoint_result_t readFlag;
    number_property_t numberProperty;
    String value;
    binary_property_t valueBinary;
}property_conf_t;

//datapoint control
typedef struct {
    dp_transmit_mode_t datapoint_transmit_mode;  // 数据点发送类型
    uint32_t datapoint_transmit_lapse;           // 数据点自动发送 时间间隔
    long runtime;                                // 数据点间隔发送时间
}datapoint_control_t;

#ifdef __cplusplus
extern "C" {
#endif

void intorobotDatapointControl(dp_transmit_mode_t mode, uint32_t lapse);
void intorobotDefineDatapointBool(const uint16_t dpID, const dp_permission_t permission, const bool value, const dp_policy_t policy, const int lapse);
void intorobotDefineDatapointNumber(const uint16_t dpID, const dp_permission_t permission, const double minValue, const double maxValue, const int resolution, const double value, const dp_policy_t policy, const int lapse);
void intorobotDefineDatapointEnum(const uint16_t dpID, const dp_permission_t permission, const int value, const dp_policy_t policy, const int lapse);
void intorobotDefineDatapointString(const uint16_t dpID, const dp_permission_t permission, const uint16_t maxLen, const char *value, const dp_policy_t policy, const int lapse);
void intorobotDefineDatapointBinary(const uint16_t dpID, const dp_permission_t permission, const uint16_t maxLen, const uint8_t *value, const uint16_t len, const dp_policy_t policy, const int lapse);

read_datapoint_result_t intorobotReadDatapointBool(const uint16_t dpID, bool &value);
read_datapoint_result_t intorobotReadDatapointInt(const uint16_t dpID, int &value);
read_datapoint_result_t intorobotReadDatapointInt32(const uint16_t dpID, int32_t &value);
read_datapoint_result_t intorobotReadDatapointUint32(const uint16_t dpID, uint32_t &value);
read_datapoint_result_t intorobotReadDatapointDouble(const uint16_t dpID, double &value);
read_datapoint_result_t intorobotReadDatapointFloat(const uint16_t dpID, float &value);
read_datapoint_result_t intorobotReadDatapointString(const uint16_t dpID, String &value);
read_datapoint_result_t intorobotReadDatapointStringChar(const uint16_t dpID, char *value);
read_datapoint_result_t intorobotReadDatapointBinary(const uint16_t dpID, uint8_t *&value, uint16_t &len);

void intorobotParseReceiveDatapoints(uint8_t *payload, uint16_t len);
void intorobotWriteDatapoint(const uint16_t dpID, const uint8_t* value, const uint16_t len, const uint8_t type );
int intorobotSendSingleDatapoint(const uint16_t dpID, const uint8_t *value, const uint16_t len, bool confirmed, uint16_t timeout);
int intorobotSendAllDatapoint(void);
int intorobotSendAllDatapointManual(bool confirmed, uint16_t timeout);
int intorobotSendDatapointAutomatic(void);

#ifdef __cplusplus
}
#endif

#endif	/* SYSTEM_CLOUDDATA_H_ */
