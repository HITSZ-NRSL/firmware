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

#ifndef WIRING_CLOUD_H_
#define WIRING_CLOUD_H_

#include "intorobot_config.h"

#ifndef configNO_CLOUD

#include <stdio.h>
#include "wiring_string.h"
//#include "events.h"
#include "system_cloud.h"
#include "system_clouddata.h"
#include "system_sleep.h"
#include "wiring_system.h"
#include "wiring_watchdog.h"
#include "interrupts_hal.h"
#include "string_convert.h"
#include <functional>
#include "service_debug.h"

class CloudClass: public Print{

    public:

        bool publish(const char *topic, bool value)
        {
            return publish(topic, String(value).c_str());
        }

#ifdef INTOROBOT_ARCH_ARM
        // stm32 arm 采取的是thumb指令集  int 实际是16位
        bool publish(const char *topic, int value)
        {
            return publish(topic, String(value).c_str());
        }
#endif

        bool publish(const char *topic, int32_t value)
        {
            return publish(topic, String(value).c_str());
        }

        bool publish(const char *topic, uint32_t value)
        {
            return publish(topic, String(value).c_str());
        }

        bool publish(const char *topic, float value)
        {
            //char buf[32];
            //char *string = dtostrf(payload, 4, 2, buf);  //如果要截取小数点后两位 调用该函数
            return publish(topic, String(value).c_str());
        }

        bool publish(const char *topic, double value)
        {
            //char buf[32];
            //char *string = dtostrf(value, 4, 2, buf);  //如果要截取小数点后两位 调用该函数
            return publish(topic, String(value).c_str());
        }

        bool publish(const char *topic, String value)
        {
            return publish(topic, value.c_str());
        }

        bool publish(const char *topic, const char *value)
        {
            return publish(topic, (uint8_t *)value, strlen(value), 0, true);
        }

        bool publish(const char *topic, uint8_t *payload, unsigned int plength)
        {
            return publish(topic, payload, plength, 0, true);
        }

        bool publish(const char *topic, uint8_t *payload, unsigned int plength, uint8_t retained)
        {
            return publish(topic, payload, plength, 0, retained);
        }

        bool publish(const char *topic, uint8_t *payload, unsigned int plength, uint8_t qos, uint8_t retained)
        {
            return intorobot_publish(API_VERSION_V1, topic, payload, plength, qos, retained);
        }

        bool subscribe(const char *topic, const char *deviceID, void (*callback)(uint8_t*, uint32_t))
        {
            return subscribe(topic, deviceID, callback, 0);
        }

        bool subscribe(const char *topic, const char *deviceID, void (*callback)(uint8_t*, uint32_t), uint8_t qos)
        {
            return intorobot_subscribe(API_VERSION_V1, topic, deviceID, callback, qos);
        }

        bool subscribe(const char *topic, const char *deviceID, WidgetBaseClass *pWidgetBase)
        {
            return subscribe(topic, deviceID, pWidgetBase, 0);
        }

        bool subscribe(const char *topic, const char *deviceID, WidgetBaseClass *pWidgetBase, uint8_t qos)
        {
            return intorobot_widget_subscribe(API_VERSION_V1, topic, deviceID, pWidgetBase, qos);
        }

        bool unsubscribe(const char *topic, const char *deviceID)
        {
            return intorobot_unsubscribe(API_VERSION_V1, topic, deviceID);
        }

        //添加通用数据收发接口
        // 添加数据点
        void addDataPointBool(const uint16_t dpID, const char *permission)
        {
            addDataPointBool(dpID, permission, false, "", 0);
        }

        void addDataPointBool(const uint16_t dpID, const char *permission, const bool value)
        {
            addDataPointBool(dpID, permission, value, "", 0);
        }

        void addDataPointBool(const uint16_t dpID, const char *permission, const bool value, const char *policy)
        {
            addDataPointBool(dpID, permission, value, policy, 0);
        }
        // lapse 单位为秒
        void addDataPointBool(const uint16_t dpID, const char *permission, const bool value, const char *policy, const int lapse)
        {
            intorobotAddDataPointBool(dpID, permission, (bool)value, policy, lapse);
        }

        void addDataPointNumber(const uint16_t dpID, const char *permission, const double minValue, const double maxValue, const double resolution)
        {
            addDataPointNumber(dpID, permission, minValue, maxValue, resolution, minValue, "", 0);
        }

        void addDataPointNumber(const uint16_t dpID, const char *permission, const double minValue, const double maxValue, const double resolution, const double value)
        {
            addDataPointNumber(dpID, permission, minValue, maxValue, resolution, value, "", 0);
        }

        void addDataPointNumber(const uint16_t dpID, const char *permission, const double minValue, const double maxValue, const double resolution, const double value, const char *policy)
        {
            addDataPointNumber(dpID, permission, minValue, maxValue, resolution, value, policy, 0);
        }

        void addDataPointNumber(const uint16_t dpID, const char *permission, const double minValue, const double maxValue, const double resolution, const double value, const char *policy, const int lapse)
        {
            intorobotAddDataPointNumber(dpID, permission, minValue, maxValue, resolution, value, policy, lapse);
        }

        void addDataPointEnum(const uint16_t dpID, const char *permission)
        {
            addDataPointEnum(dpID, permission, 0, "", 0);
        }

        void addDataPointEnum(const uint16_t dpID, const char *permission, const int value)
        {
            addDataPointEnum(dpID, permission, value, "", 0);
        }

        void addDataPointEnum(const uint16_t dpID, const char *permission, const int value, const char *policy)
        {
            addDataPointEnum(dpID, permission, value, policy, 0);
        }

        void addDataPointEnum(const uint16_t dpID, const char *permission, const int value, const char *policy, const int lapse)
        {
            intorobotAddDataPointEnum(dpID, permission, value, policy, lapse);
        }

        void addDataPointString(const uint16_t dpID, const char *permission)
        {
            addDataPointString(dpID, permission, "", "", 0);
        }

        void addDataPointString(const uint16_t dpID, const char *permission, const char *value)
        {
            addDataPointString(dpID, permission, value, "", 0);
        }

        void addDataPointString(const uint16_t dpID, const char *permission, const char *value, const char *policy)
        {
            addDataPointString(dpID, permission, value, policy, 0);
        }

        void addDataPointString(const uint16_t dpID, const char *permission, const char *value, const char *policy, const int lapse)
        {
            intorobotAddDataPointString(dpID, permission, value, policy, lapse);
        }

        void addDataPointBinary(const uint16_t dpID, const char *permission)
        {
            addDataPointBinary(dpID, permission, NULL, 0, "", 0);
        }

        void addDataPointBinary(const uint16_t dpID, const char *permission, const uint8_t *value, const uint16_t len)
        {
            addDataPointBinary(dpID, permission, value, len, "", 0);
        }

        void addDataPointBinary(const uint16_t dpID, const char *permission, const uint8_t *value, const uint16_t len, const char *policy)
        {
            addDataPointBinary(dpID, permission, value, len, policy, 0);
        }

        void addDataPointBinary(const uint16_t dpID, const char *permission, const uint8_t *value, const uint16_t len, const char *policy, const int lapse)
        {
            intorobotAddDataPointBinary(dpID, permission, value, len, policy, lapse);
        }

        // 读取数据点
        read_datapoint_result_t readDataPoint(const uint16_t dpID, bool &value)
        {
            return intorobotReadDataPointBool(dpID, value);
        }

#ifdef INTOROBOT_ARCH_ARM
        read_datapoint_result_t readDataPoint(const uint16_t dpID, int &value)
        {
            return intorobotReadDataPointInt(dpID, value);
        }
#endif

        read_datapoint_result_t readDataPoint(const uint16_t dpID, int32_t &value)
        {
            return intorobotReadDataPointInt32(dpID, value);
        }

        read_datapoint_result_t readDataPoint(const uint16_t dpID, uint32_t &value)
        {
            return intorobotReadDataPointUint32(dpID, value);
        }

        read_datapoint_result_t readDataPoint(const uint16_t dpID, float &value)
        {
            return intorobotReadDataPointFloat(dpID, value);
        }

        read_datapoint_result_t readDataPoint(const uint16_t dpID, double &value)
        {
            return intorobotReadDataPointDouble(dpID, value);
        }

        read_datapoint_result_t readDataPoint(const uint16_t dpID, String &value)
        {
            return intorobotReadDataPointString(dpID, value);
        }

        read_datapoint_result_t readDataPoint(const uint16_t dpID, char *value)
        {
            return intorobotReadDataPointStringChar(dpID, value);
        }

        read_datapoint_result_t readDataPoint(const uint16_t dpID, uint8_t *value, uint16_t &len)
        {
            return intorobotReadDataPointBinary(dpID, value, len);
        }

        // 写数据点
        void writeDataPoint(const uint16_t dpID, bool value)
        {
            writeDataPoint(dpID, String(value).c_str());
        }

#ifdef INTOROBOT_ARCH_ARM
        void writeDataPoint(const uint16_t dpID, int value)
        {
            writeDataPoint(dpID, String(value).c_str());
        }
#endif
        void writeDataPoint(const uint16_t dpID, int32_t value)
        {
            writeDataPoint(dpID, String(value).c_str());
        }

        void writeDataPoint(const uint16_t dpID, uint32_t value)
        {
            writeDataPoint(dpID, String(value).c_str());
        }

        void writeDataPoint(const uint16_t dpID, float value)
        {
            writeDataPoint(dpID, String(value).c_str());
        }

        void writeDataPoint(const uint16_t dpID, double value)
        {
            writeDataPoint(dpID, String(value).c_str());
        }

        void writeDataPoint(const uint16_t dpID, String value)
        {
            writeDataPoint(dpID, value.c_str());
        }

        void writeDataPoint(const uint16_t dpID, const char *value)
        {
            intorobotWriteDataPointString(dpID, value);
        }

        void writeDataPoint(const uint16_t dpID, uint8_t *value, uint16_t len)
        {
            intorobotWriteDataPointBinary(dpID, value, len);
        }

        void writeDataPointAll(void)
        {
            intorobotWriteDataPointAll();
        }

        void syncTime(void)
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
        static String deviceID(void) { return intorobot_deviceID(); }

        int read(void)
        {
            return intorobot_debug_info_read();
        }
        int available(void)
        {
            return intorobot_debug_info_available();
        }

    private:
        virtual size_t write(uint8_t byte)
        {return intorobot_debug_info_write(byte);}

        using Print::write; // pull in write(str) and write(buf, size) from Print
};

extern CloudClass IntoRobot;

#endif

#endif
