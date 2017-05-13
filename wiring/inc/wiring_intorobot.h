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

class IntoRobotDatepointClass {

    public:
        //添加通用数据收发接口
        // 添加数据点
        static void defineDatapointBool(const uint16_t dpID, const char *permission)
        {
            defineDatapointBool(dpID, permission, false, "", 0);
        }

        static void defineDatapointBool(const uint16_t dpID, const char *permission, const bool value)
        {
            defineDatapointBool(dpID, permission, value, "", 0);
        }

        static void defineDatapointBool(const uint16_t dpID, const char *permission, const bool value, const char *policy)
        {
            defineDatapointBool(dpID, permission, value, policy, 0);
        }
        // lapse 单位为秒
        static void defineDatapointBool(const uint16_t dpID, const char *permission, const bool value, const char *policy, const int lapse)
        {
            intorobotDefineDatapointBool(dpID, permission, (bool)value, policy, lapse);
        }

        static void defineDatapointNumber(const uint16_t dpID, const char *permission, const double minValue, const double maxValue, const int resolution)
        {
            defineDatapointNumber(dpID, permission, minValue, maxValue, resolution, minValue, "", 0);
        }

        static void defineDatapointNumber(const uint16_t dpID, const char *permission, const double minValue, const double maxValue, const int resolution, const double value)
        {
            defineDatapointNumber(dpID, permission, minValue, maxValue, resolution, value, "", 0);
        }

        static void defineDatapointNumber(const uint16_t dpID, const char *permission, const double minValue, const double maxValue, const int resolution, const double value, const char *policy)
        {
            defineDatapointNumber(dpID, permission, minValue, maxValue, resolution, value, policy, 0);
        }

        static void defineDatapointNumber(const uint16_t dpID, const char *permission, const double minValue, const double maxValue, const int resolution, const double value, const char *policy, const int lapse)
        {
            intorobotDefineDatapointNumber(dpID, permission, minValue, maxValue, resolution, value, policy, lapse);
        }

        static void defineDatapointEnum(const uint16_t dpID, const char *permission)
        {
            defineDatapointEnum(dpID, permission, 0, "", 0);
        }

        static void defineDatapointEnum(const uint16_t dpID, const char *permission, const int value)
        {
            defineDatapointEnum(dpID, permission, value, "", 0);
        }

        static void defineDatapointEnum(const uint16_t dpID, const char *permission, const int value, const char *policy)
        {
            defineDatapointEnum(dpID, permission, value, policy, 0);
        }

        static void defineDatapointEnum(const uint16_t dpID, const char *permission, const int value, const char *policy, const int lapse)
        {
            intorobotDefineDatapointEnum(dpID, permission, value, policy, lapse);
        }

        static void defineDatapointString(const uint16_t dpID, const char *permission)
        {
            defineDatapointString(dpID, permission, "", "", 0);
        }

        static void defineDatapointString(const uint16_t dpID, const char *permission, const char *value)
        {
            defineDatapointString(dpID, permission, value, "", 0);
        }

        static void defineDatapointString(const uint16_t dpID, const char *permission, const char *value, const char *policy)
        {
            defineDatapointString(dpID, permission, value, policy, 0);
        }

        static void defineDatapointString(const uint16_t dpID, const char *permission, const char *value, const char *policy, const int lapse)
        {
            intorobotDefineDatapointString(dpID, permission, value, policy, lapse);
        }

        static void defineDatapointBinary(const uint16_t dpID, const char *permission)
        {
            defineDatapointBinary(dpID, permission, NULL, 0, "", 0);
        }

        static void defineDatapointBinary(const uint16_t dpID, const char *permission, const uint8_t *value, const uint16_t len)
        {
            defineDatapointBinary(dpID, permission, value, len, "", 0);
        }

        static void defineDatapointBinary(const uint16_t dpID, const char *permission, const uint8_t *value, const uint16_t len, const char *policy)
        {
            defineDatapointBinary(dpID, permission, value, len, policy, 0);
        }

        static void defineDatapointBinary(const uint16_t dpID, const char *permission, const uint8_t *value, const uint16_t len, const char *policy, const int lapse)
        {
            intorobotDefineDatapointBinary(dpID, permission, value, len, policy, lapse);
        }

        // 读取数据点
        static read_datapoint_result_t readDatapoint(const uint16_t dpID, bool &value)
        {
            return intorobotReadDatapointBool(dpID, value);
        }

#ifdef INTOROBOT_ARCH_ARM
        static read_datapoint_result_t readDatapoint(const uint16_t dpID, int &value)
        {
            return intorobotReadDatapointInt(dpID, value);
        }
#endif

        static read_datapoint_result_t readDatapoint(const uint16_t dpID, int32_t &value)
        {
            return intorobotReadDatapointInt32(dpID, value);
        }

        static read_datapoint_result_t readDatapoint(const uint16_t dpID, uint32_t &value)
        {
            return intorobotReadDatapointUint32(dpID, value);
        }

        static read_datapoint_result_t readDatapoint(const uint16_t dpID, float &value)
        {
            return intorobotReadDatapointFloat(dpID, value);
        }

        static read_datapoint_result_t readDatapoint(const uint16_t dpID, double &value)
        {
            return intorobotReadDatapointDouble(dpID, value);
        }

        static read_datapoint_result_t readDatapoint(const uint16_t dpID, String &value)
        {
            return intorobotReadDatapointString(dpID, value);
        }

        static read_datapoint_result_t readDatapoint(const uint16_t dpID, char *value)
        {
            return intorobotReadDatapointStringChar(dpID, value);
        }

        static read_datapoint_result_t readDatapoint(const uint16_t dpID, uint8_t *value, uint16_t &len)
        {
            return intorobotReadDatapointBinary(dpID, value, len);
        }

        // 写数据点
        static void writeDatapoint(const uint16_t dpID, bool value)
        {
            writeDatapoint(dpID, String(value).c_str());
        }

#ifdef INTOROBOT_ARCH_ARM
        static void writeDatapoint(const uint16_t dpID, int value)
        {
            writeDatapoint(dpID, String(value).c_str());
        }
#endif
        static void writeDatapoint(const uint16_t dpID, int32_t value)
        {
            writeDatapoint(dpID, String(value).c_str());
        }

        static void writeDatapoint(const uint16_t dpID, uint32_t value)
        {
            writeDatapoint(dpID, String(value).c_str());
        }

        static void writeDatapoint(const uint16_t dpID, float value)
        {
            writeDatapoint(dpID, String(value).c_str());
        }

        static void writeDatapoint(const uint16_t dpID, double value)
        {
            writeDatapoint(dpID, String(value).c_str());
        }

        static void writeDatapoint(const uint16_t dpID, String value)
        {
            writeDatapoint(dpID, value.c_str());
        }

        static void writeDatapoint(const uint16_t dpID, const char *value)
        {
            intorobotWriteDatapoint(dpID, value);
        }

        static void writeDatapoint(const uint16_t dpID, uint8_t *value, uint16_t len)
        {
            //todo
        }

        // 发送数据点
        static void sendDatapoint(const uint16_t dpID, bool value)
        {
            sendDatapoint(dpID, String(value).c_str());
        }

#ifdef INTOROBOT_ARCH_ARM
        static void sendDatapoint(const uint16_t dpID, int value)
        {
            sendDatapoint(dpID, String(value).c_str());
        }
#endif
        static void sendDatapoint(const uint16_t dpID, int32_t value)
        {
            sendDatapoint(dpID, String(value).c_str());
        }

        static void sendDatapoint(const uint16_t dpID, uint32_t value)
        {
            sendDatapoint(dpID, String(value).c_str());
        }

        static void sendDatapoint(const uint16_t dpID, float value)
        {
            sendDatapoint(dpID, String(value).c_str());
        }

        static void sendDatapoint(const uint16_t dpID, double value)
        {
            sendDatapoint(dpID, String(value).c_str());
        }

        static void sendDatapoint(const uint16_t dpID, String value)
        {
            sendDatapoint(dpID, value.c_str());
        }

        static void sendDatapoint(const uint16_t dpID, const char *value)
        {
            intorobotSendDatapoint(dpID, value);
        }

        static void sendDatapoint(const uint16_t dpID, uint8_t *value, uint16_t len)
        {
            //todo
        }

        static void sendDatapointAll(void)
        {
            intorobotSendDatapointAll();
        }

        //数据点组处理  todo
        static void cleanDatapointGroup(uint8_t group = 0)
        {
        }

        static void addDatapointToGroup(const uint16_t dpID, uint8_t group = 0)
        {
        }

        static void sendDatapointGroup(uint8_t group = 0)
        {
        }
};

#ifndef configNO_CLOUD
class IntoRobotClass: public IntoRobotDatepointClass, public Print{

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
class IntoRobotClass: public IntoRobotDatepointClass {

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

