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

#ifndef WIRING_INTOROBOT_H_
#define WIRING_INTOROBOT_H_

#include "intorobot_config.h"
#include <stdio.h>
#include "wiring_string.h"
//#include "events.h"
#include "system_cloud.h"
#include "system_lorawan.h"
#include "system_datapoint.h"
#include "system_sleep.h"
#include "wiring_system.h"
#include "wiring_watchdog.h"
#include "interrupts_hal.h"
#include "string_convert.h"
#include <functional>
#include "service_debug.h"

class IntoRobotDatePointClass {

    public:
        //添加通用数据收发接口
        // 添加数据点
        static void defineDataPointBool(const uint16_t dpID, const char *permission)
        {
            defineDataPointBool(dpID, permission, false, "", 0);
        }

        static void defineDataPointBool(const uint16_t dpID, const char *permission, const bool value)
        {
            defineDataPointBool(dpID, permission, value, "", 0);
        }

        static void defineDataPointBool(const uint16_t dpID, const char *permission, const bool value, const char *policy)
        {
            defineDataPointBool(dpID, permission, value, policy, 0);
        }
        // lapse 单位为秒
        static void defineDataPointBool(const uint16_t dpID, const char *permission, const bool value, const char *policy, const int lapse)
        {
            intorobotDefineDataPointBool(dpID, permission, (bool)value, policy, lapse);
        }

        static void defineDataPointNumber(const uint16_t dpID, const char *permission, const double minValue, const double maxValue, const int resolution)
        {
            defineDataPointNumber(dpID, permission, minValue, maxValue, resolution, minValue, "", 0);
        }

        static void defineDataPointNumber(const uint16_t dpID, const char *permission, const double minValue, const double maxValue, const int resolution, const double value)
        {
            defineDataPointNumber(dpID, permission, minValue, maxValue, resolution, value, "", 0);
        }

        static void defineDataPointNumber(const uint16_t dpID, const char *permission, const double minValue, const double maxValue, const int resolution, const double value, const char *policy)
        {
            defineDataPointNumber(dpID, permission, minValue, maxValue, resolution, value, policy, 0);
        }

        static void defineDataPointNumber(const uint16_t dpID, const char *permission, const double minValue, const double maxValue, const int resolution, const double value, const char *policy, const int lapse)
        {
            intorobotDefineDataPointNumber(dpID, permission, minValue, maxValue, resolution, value, policy, lapse);
        }

        static void defineDataPointEnum(const uint16_t dpID, const char *permission)
        {
            defineDataPointEnum(dpID, permission, 0, "", 0);
        }

        static void defineDataPointEnum(const uint16_t dpID, const char *permission, const int value)
        {
            defineDataPointEnum(dpID, permission, value, "", 0);
        }

        static void defineDataPointEnum(const uint16_t dpID, const char *permission, const int value, const char *policy)
        {
            defineDataPointEnum(dpID, permission, value, policy, 0);
        }

        static void defineDataPointEnum(const uint16_t dpID, const char *permission, const int value, const char *policy, const int lapse)
        {
            intorobotDefineDataPointEnum(dpID, permission, value, policy, lapse);
        }

        static void defineDataPointString(const uint16_t dpID, const char *permission)
        {
            defineDataPointString(dpID, permission, "", "", 0);
        }

        static void defineDataPointString(const uint16_t dpID, const char *permission, const char *value)
        {
            defineDataPointString(dpID, permission, value, "", 0);
        }

        static void defineDataPointString(const uint16_t dpID, const char *permission, const char *value, const char *policy)
        {
            defineDataPointString(dpID, permission, value, policy, 0);
        }

        static void defineDataPointString(const uint16_t dpID, const char *permission, const char *value, const char *policy, const int lapse)
        {
            intorobotDefineDataPointString(dpID, permission, value, policy, lapse);
        }

        static void defineDataPointBinary(const uint16_t dpID, const char *permission)
        {
            defineDataPointBinary(dpID, permission, NULL, 0, "", 0);
        }

        static void defineDataPointBinary(const uint16_t dpID, const char *permission, const uint8_t *value, const uint16_t len)
        {
            defineDataPointBinary(dpID, permission, value, len, "", 0);
        }

        static void defineDataPointBinary(const uint16_t dpID, const char *permission, const uint8_t *value, const uint16_t len, const char *policy)
        {
            defineDataPointBinary(dpID, permission, value, len, policy, 0);
        }

        static void defineDataPointBinary(const uint16_t dpID, const char *permission, const uint8_t *value, const uint16_t len, const char *policy, const int lapse)
        {
            intorobotDefineDataPointBinary(dpID, permission, value, len, policy, lapse);
        }

        // 读取数据点
        static read_datapoint_result_t readDataPoint(const uint16_t dpID, bool &value)
        {
            return intorobotReadDataPointBool(dpID, value);
        }

#ifdef INTOROBOT_ARCH_ARM
        static read_datapoint_result_t readDataPoint(const uint16_t dpID, int &value)
        {
            return intorobotReadDataPointInt(dpID, value);
        }
#endif

        static read_datapoint_result_t readDataPoint(const uint16_t dpID, int32_t &value)
        {
            return intorobotReadDataPointInt32(dpID, value);
        }

        static read_datapoint_result_t readDataPoint(const uint16_t dpID, uint32_t &value)
        {
            return intorobotReadDataPointUint32(dpID, value);
        }

        static read_datapoint_result_t readDataPoint(const uint16_t dpID, float &value)
        {
            return intorobotReadDataPointFloat(dpID, value);
        }

        static read_datapoint_result_t readDataPoint(const uint16_t dpID, double &value)
        {
            return intorobotReadDataPointDouble(dpID, value);
        }

        static read_datapoint_result_t readDataPoint(const uint16_t dpID, String &value)
        {
            return intorobotReadDataPointString(dpID, value);
        }

        static read_datapoint_result_t readDataPoint(const uint16_t dpID, char *value)
        {
            return intorobotReadDataPointStringChar(dpID, value);
        }

        static read_datapoint_result_t readDataPoint(const uint16_t dpID, uint8_t *value, uint16_t &len)
        {
            return intorobotReadDataPointBinary(dpID, value, len);
        }

        // 写数据点
        static void writeDataPoint(const uint16_t dpID, bool value)
        {
            writeDataPoint(dpID, String(value).c_str());
        }

#ifdef INTOROBOT_ARCH_ARM
        static void writeDataPoint(const uint16_t dpID, int value)
        {
            writeDataPoint(dpID, String(value).c_str());
        }
#endif
        static void writeDataPoint(const uint16_t dpID, int32_t value)
        {
            writeDataPoint(dpID, String(value).c_str());
        }

        static void writeDataPoint(const uint16_t dpID, uint32_t value)
        {
            writeDataPoint(dpID, String(value).c_str());
        }

        static void writeDataPoint(const uint16_t dpID, float value)
        {
            writeDataPoint(dpID, String(value).c_str());
        }

        static void writeDataPoint(const uint16_t dpID, double value)
        {
            writeDataPoint(dpID, String(value).c_str());
        }

        static void writeDataPoint(const uint16_t dpID, String value)
        {
            writeDataPoint(dpID, value.c_str());
        }

        static void writeDataPoint(const uint16_t dpID, const char *value)
        {
            intorobotWriteDataPoint(dpID, value);
        }

        static void writeDataPoint(const uint16_t dpID, uint8_t *value, uint16_t len)
        {
            //todo
        }

        // 发送数据点
        static void sendDataPoint(const uint16_t dpID, bool value)
        {
            sendDataPoint(dpID, String(value).c_str());
        }

#ifdef INTOROBOT_ARCH_ARM
        static void sendDataPoint(const uint16_t dpID, int value)
        {
            sendDataPoint(dpID, String(value).c_str());
        }
#endif
        static void sendDataPoint(const uint16_t dpID, int32_t value)
        {
            sendDataPoint(dpID, String(value).c_str());
        }

        static void sendDataPoint(const uint16_t dpID, uint32_t value)
        {
            sendDataPoint(dpID, String(value).c_str());
        }

        static void sendDataPoint(const uint16_t dpID, float value)
        {
            sendDataPoint(dpID, String(value).c_str());
        }

        static void sendDataPoint(const uint16_t dpID, double value)
        {
            sendDataPoint(dpID, String(value).c_str());
        }

        static void sendDataPoint(const uint16_t dpID, String value)
        {
            sendDataPoint(dpID, value.c_str());
        }

        static void sendDataPoint(const uint16_t dpID, const char *value)
        {
            intorobotSendDataPoint(dpID, value);
        }

        static void sendDataPoint(const uint16_t dpID, uint8_t *value, uint16_t len)
        {
            //todo
        }

        static void sendDataPointAll(void)
        {
            intorobotSendDataPointAll();
        }

        //数据点组处理  todo
        static void cleanDataPointGroup(uint8_t group = 0)
        {
        }

        static void addDataPointToGroup(const uint16_t dpID, uint8_t group = 0)
        {
        }

        static void sendDataPointGroup(uint8_t group = 0)
        {
        }
};

#ifndef configNO_CLOUD
class IntoRobotClass: public IntoRobotDatePointClass, Print{

    public:

        static bool publish(const char *topic, bool value)
        {
            return publish(topic, String(value).c_str());
        }

#ifdef INTOROBOT_ARCH_ARM
        // stm32 arm 采取的是thumb指令集  int 实际是16位
        static bool publish(const char *topic, int value)
        {
            return publish(topic, String(value).c_str());
        }
#endif

        static bool publish(const char *topic, int32_t value)
        {
            return publish(topic, String(value).c_str());
        }

        static bool publish(const char *topic, uint32_t value)
        {
            return publish(topic, String(value).c_str());
        }

        static bool publish(const char *topic, float value)
        {
            //char buf[32];
            //char *string = dtostrf(payload, 4, 2, buf);  //如果要截取小数点后两位 调用该函数
            return publish(topic, String(value).c_str());
        }

        static bool publish(const char *topic, double value)
        {
            //char buf[32];
            //char *string = dtostrf(value, 4, 2, buf);  //如果要截取小数点后两位 调用该函数
            return publish(topic, String(value).c_str());
        }

        static bool publish(const char *topic, String value)
        {
            return publish(topic, value.c_str());
        }

        static bool publish(const char *topic, const char *value)
        {
            return publish(topic, (uint8_t *)value, strlen(value), 0, true);
        }

        static bool publish(const char *topic, uint8_t *payload, unsigned int plength)
        {
            return publish(topic, payload, plength, 0, true);
        }

        static bool publish(const char *topic, uint8_t *payload, unsigned int plength, uint8_t retained)
        {
            return publish(topic, payload, plength, 0, retained);
        }

        static bool publish(const char *topic, uint8_t *payload, unsigned int plength, uint8_t qos, uint8_t retained)
        {
            return intorobot_publish(API_VERSION_V1, topic, payload, plength, qos, retained);
        }

        static bool subscribe(const char *topic, const char *deviceID, void (*callback)(uint8_t*, uint32_t))
        {
            return subscribe(topic, deviceID, callback, 0);
        }

        static bool subscribe(const char *topic, const char *deviceID, void (*callback)(uint8_t*, uint32_t), uint8_t qos)
        {
            return intorobot_subscribe(API_VERSION_V1, topic, deviceID, callback, qos);
        }

        static bool subscribe(const char *topic, const char *deviceID, WidgetBaseClass *pWidgetBase)
        {
            return subscribe(topic, deviceID, pWidgetBase, 0);
        }

        static bool subscribe(const char *topic, const char *deviceID, WidgetBaseClass *pWidgetBase, uint8_t qos)
        {
            return intorobot_widget_subscribe(API_VERSION_V1, topic, deviceID, pWidgetBase, qos);
        }

        static bool unsubscribe(const char *topic, const char *deviceID)
        {
            return intorobot_unsubscribe(API_VERSION_V1, topic, deviceID);
        }

        static void syncTime(void)
        {
            intorobot_sync_time();
        }

        static bool connected(void) { return intorobot_cloud_flag_connected(); }
        static bool disconnected(void) { return !connected(); }
        static void connect(void) {
            intorobot_cloud_flag_connect();
            if (system_thread_get_state(nullptr)==intorobot::feature::DISABLED &&
                    SystemClass::mode() == SEMI_AUTOMATIC)
            {
                // IntoRobot.connect() should be blocking in SEMI_AUTOMATIC mode when threading is disabled
                waitUntil(connected);
            }
        }
        static void disconnect(void) { intorobot_cloud_flag_disconnect(); }
        static void process(void) {
            //application_checkin();
            intorobot_process();
        }

        static int read(void)
        {
            return intorobot_debug_info_read();
        }
        static int available(void)
        {
            return intorobot_debug_info_available();
        }

        static String deviceID(void) { return intorobot_deviceID(); }

    private:
        virtual size_t write(uint8_t byte)
        {return intorobot_debug_info_write(byte);}

        using Print::write; // pull in write(str) and write(buf, size) from Print
};

#endif

#ifndef configNO_LORAWAN
class IntoRobotClass: public IntoRobotDatePointClass {

    public:
        static void process(void) {
            //application_checkin();
            intorobot_process();
        }

        static String deviceID(void) { return intorobot_deviceID(); }

};
#endif

#if (!defined configNO_CLOUD) || (!defined configNO_LORAWAN)

IntoRobotClass& __fetch_global_intorobot();
#define IntoRobot __fetch_global_intorobot()
#define IntoYun __fetch_global_intorobot()

#endif

#endif

