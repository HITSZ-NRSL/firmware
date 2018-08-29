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
#include <stdio.h>
#include "wiring_string.h"
#include "system_cloud.h"
#include "system_lorawan.h"
#include "system_datapoint.h"
#include "system_sleep.h"
#include "wiring_system.h"
#include "wiring_lorawan.h"
#include "wiring_watchdog.h"
#include "interrupts_hal.h"
#include "string_convert.h"
#include <functional>
#include "molmc_log.h"

class CloudDatepointClass {
    public:
        //数据点自动发送设置
        static void datapointControl(dp_transmit_mode_t mode) {
#ifndef configNO_CLOUD
            intorobotDatapointControl(mode, DATAPOINT_TRANSMIT_AUTOMATIC_INTERVAL);
#endif
        }
        static void datapointControl(dp_transmit_mode_t mode, uint32_t lapse) { //控制数据点  单位为s
#ifndef configNO_CLOUD
            intorobotDatapointControl(mode, lapse);
#endif
        }

        // 添加数据点
        static void defineDatapointBool(const uint16_t dpID, const dp_permission_t permission) {
            defineDatapointBool(dpID, permission, false);
        }
        static void defineDatapointBool(const uint16_t dpID, const dp_permission_t permission, const bool value) {
            intorobotDefineDatapointBool(dpID, permission, value, DP_POLICY_NONE, 0);
        }

        static void defineDatapointNumber(const uint16_t dpID, const dp_permission_t permission, const double minValue, const double maxValue, const int resolution) {
            defineDatapointNumber(dpID, permission, minValue, maxValue, resolution, minValue);
        }
        static void defineDatapointNumber(const uint16_t dpID, const dp_permission_t permission, const double minValue, const double maxValue, const int resolution, const double value) {
            intorobotDefineDatapointNumber(dpID, permission, minValue, maxValue, resolution, value, DP_POLICY_NONE, 0);
        }

        static void defineDatapointEnum(const uint16_t dpID, const dp_permission_t permission) {
            defineDatapointEnum(dpID, permission, 0);
        }
        static void defineDatapointEnum(const uint16_t dpID, const dp_permission_t permission, const int value) {
            intorobotDefineDatapointEnum(dpID, permission, value, DP_POLICY_NONE, 0);
        }

        static void defineDatapointString(const uint16_t dpID, const dp_permission_t permission, const uint16_t maxLen) {
            defineDatapointString(dpID, permission, maxLen, "");
        }
        static void defineDatapointString(const uint16_t dpID, const dp_permission_t permission, const uint16_t maxLen, const char *value) {
            intorobotDefineDatapointString(dpID, permission, maxLen, value, DP_POLICY_NONE, 0);
        }

        static void defineDatapointBinary(const uint16_t dpID, const dp_permission_t permission, const uint16_t maxLen ) {
            defineDatapointBinary(dpID, permission, maxLen, NULL, 0);
        }
        static void defineDatapointBinary(const uint16_t dpID, const dp_permission_t permission, const uint16_t maxLen, const uint8_t *value, const uint16_t len) {
            intorobotDefineDatapointBinary(dpID, permission, maxLen, value, len, DP_POLICY_NONE, 0);
        }

        // 读取数据点
        static read_datapoint_result_t readDatapoint(const uint16_t dpID, bool &value) {
            return intorobotReadDatapointBool(dpID, value);
        }
#ifdef INTOROBOT_ARCH_ARM
        static read_datapoint_result_t readDatapoint(const uint16_t dpID, int &value) {
            return intorobotReadDatapointInt(dpID, value);
        }
#endif
        static read_datapoint_result_t readDatapoint(const uint16_t dpID, int32_t &value) {
            return intorobotReadDatapointInt32(dpID, value);
        }
        static read_datapoint_result_t readDatapoint(const uint16_t dpID, uint32_t &value) {
            return intorobotReadDatapointUint32(dpID, value);
        }
        static read_datapoint_result_t readDatapoint(const uint16_t dpID, float &value) {
            return intorobotReadDatapointFloat(dpID, value);
        }
        static read_datapoint_result_t readDatapoint(const uint16_t dpID, double &value) {
            return intorobotReadDatapointDouble(dpID, value);
        }
        static read_datapoint_result_t readDatapoint(const uint16_t dpID, String &value) {
            return intorobotReadDatapointString(dpID, value);
        }
        static read_datapoint_result_t readDatapoint(const uint16_t dpID, char *value) {
            return intorobotReadDatapointStringChar(dpID, value);
        }
        static read_datapoint_result_t readDatapoint(const uint16_t dpID, uint8_t *&value, uint16_t &len) {
            return intorobotReadDatapointBinary(dpID, value, len);
        }

        // 写数据点
        static void writeDatapoint(const uint16_t dpID, bool value) {
            writeDatapoint(dpID, String(value).c_str());
        }
#ifdef INTOROBOT_ARCH_ARM
        static void writeDatapoint(const uint16_t dpID, int value) {
            writeDatapoint(dpID, String(value).c_str());
        }
#endif
        static void writeDatapoint(const uint16_t dpID, int32_t value) {
            writeDatapoint(dpID, String(value).c_str());
        }
        static void writeDatapoint(const uint16_t dpID, uint32_t value) {
            writeDatapoint(dpID, String(value).c_str());
        }
        static void writeDatapoint(const uint16_t dpID, float value) {
            writeDatapoint(dpID, String(value).c_str());
        }
        static void writeDatapoint(const uint16_t dpID, double value) {
            writeDatapoint(dpID, String(value).c_str());
        }
        static void writeDatapoint(const uint16_t dpID, String value) {
            writeDatapoint(dpID, value.c_str());
        }
        static void writeDatapoint(const uint16_t dpID, const char *value) {
            intorobotWriteDatapoint(dpID, (uint8_t *)value, strlen(value), 1);
        }
        static void writeDatapoint(const uint16_t dpID, uint8_t *value, uint16_t len) {
            intorobotWriteDatapoint(dpID, value, len, 1);
        }

        // 发送数据点
        static int sendDatapoint(const uint16_t dpID, bool value, bool confirmed = false, uint16_t timeout = 0) {
            return sendDatapoint(dpID, String(value).c_str(), confirmed, timeout);
        }
#ifdef INTOROBOT_ARCH_ARM
        static int sendDatapoint(const uint16_t dpID, int value, bool confirmed = false, uint16_t timeout = 0) {
            return sendDatapoint(dpID, String(value).c_str(), confirmed, timeout);
        }
#endif
        static int sendDatapoint(const uint16_t dpID, int32_t value, bool confirmed = false, uint16_t timeout = 0) {
            return sendDatapoint(dpID, String(value).c_str(), confirmed, timeout);
        }
        static int sendDatapoint(const uint16_t dpID, uint32_t value, bool confirmed = false, uint16_t timeout = 0) {
            return sendDatapoint(dpID, String(value).c_str(), confirmed, timeout);
        }
        static int sendDatapoint(const uint16_t dpID, float value, bool confirmed = false, uint16_t timeout = 0) {
            return sendDatapoint(dpID, String(value).c_str(), confirmed, timeout);
        }
        static int sendDatapoint(const uint16_t dpID, double value, bool confirmed = false, uint16_t timeout = 0) {
            return sendDatapoint(dpID, String(value).c_str(), confirmed, timeout);
        }
        static int sendDatapoint(const uint16_t dpID, String value, bool confirmed = false, uint16_t timeout = 0) {
            return sendDatapoint(dpID, value.c_str(), confirmed, timeout);
        }
        static int sendDatapoint(const uint16_t dpID, const char *value, bool confirmed = false, uint16_t timeout = 0) {
            return sendDatapoint(dpID, (uint8_t *)value, strlen(value), confirmed, timeout);
        }
        static int sendDatapoint(const uint16_t dpID, uint8_t *value, uint16_t len, bool confirmed = false, uint16_t timeout = 0) {
            return intorobotSendSingleDatapoint(dpID, value, len, confirmed, timeout);
        }
        static int sendDatapointAll(void) {
            return sendDatapointAll(false, 0);
        }
        static int sendDatapointAll(bool confirmed, uint16_t timeout) {
            return intorobotSendAllDatapointManual(confirmed, timeout);
        }
        // 发送自定义数据
        static int sendCustomData(const uint8_t *buffer, uint16_t len, bool confirmed = false, uint16_t timeout = 0) {
            return intorobotSendRawData(buffer, len, confirmed, timeout);
        }
};

#ifndef configNO_CLOUD
class CloudClass: public CloudDatepointClass, public Print{
    public:
        static bool publish(const char *topic, bool value) {
            return publish(topic, String(value).c_str());
        }
#ifdef INTOROBOT_ARCH_ARM
        // stm32 arm 采取的是thumb指令集  int 实际是16位
        static bool publish(const char *topic, int value) {
            return publish(topic, String(value).c_str());
        }
#endif
        static bool publish(const char *topic, int32_t value) {
            return publish(topic, String(value).c_str());
        }
        static bool publish(const char *topic, uint32_t value) {
            return publish(topic, String(value).c_str());
        }
        static bool publish(const char *topic, float value) {
            return publish(topic, String(value).c_str());
        }
        static bool publish(const char *topic, double value) {
            return publish(topic, String(value).c_str());
        }
        static bool publish(const char *topic, String value) {
            return publish(topic, value.c_str());
        }
        static bool publish(const char *topic, const char *value) {
            return publish(topic, (uint8_t *)value, strlen(value));
        }
        static bool publish(const char *topic, uint8_t *payload, unsigned int plength) {
            return publish(topic, payload, plength, 0, true, TOPIC_VERSION_V1);
        }
        static bool publish(const char *topic, uint8_t *payload, unsigned int plength, uint8_t retained) {
            return publish(topic, payload, plength, 0, retained, TOPIC_VERSION_V1);
        }
        static bool publish(const char *topic, uint8_t *payload, unsigned int plength, uint8_t qos, uint8_t retained, topic_version_t version) {
            return intorobot_publish(version, topic, payload, plength, qos, retained);
        }

        static bool subscribe(const char *topic, const char *deviceID, void (*callback)(uint8_t*, uint32_t)) {
            return subscribe(topic, deviceID, callback, 0, TOPIC_VERSION_V1);
        }
        static bool subscribe(const char *topic, const char *deviceID, void (*callback)(uint8_t*, uint32_t), uint8_t qos) {
            return subscribe(topic, deviceID, callback, qos, TOPIC_VERSION_V1);
        }
        static bool subscribe(const char *topic, const char *deviceID, void (*callback)(uint8_t*, uint32_t), uint8_t qos, topic_version_t version) {
            return intorobot_subscribe(version, topic, deviceID, callback, qos);
        }
        static bool subscribe(const char *topic, const char *deviceID, WidgetBaseClass *pWidgetBase) {
            return subscribe(topic, deviceID, pWidgetBase, 0);
        }
        static bool subscribe(const char *topic, const char *deviceID, WidgetBaseClass *pWidgetBase, uint8_t qos) {
            return subscribe(topic, deviceID, pWidgetBase, qos, TOPIC_VERSION_V1);
        }
        static bool subscribe(const char *topic, const char *deviceID, WidgetBaseClass *pWidgetBase, uint8_t qos, topic_version_t version) {
            return intorobot_widget_subscribe(version, topic, deviceID, pWidgetBase, qos);
        }

        static bool unsubscribe(const char *topic, const char *deviceID) {
            return unsubscribe(topic, deviceID, TOPIC_VERSION_V1);
        }
        static bool unsubscribe(const char *topic, const char *deviceID, topic_version_t version) {
            return intorobot_unsubscribe(version, topic, deviceID);
        }

        static bool syncTime(void) {
            return intorobot_sync_time();
        }

        static bool connected(void) {return intorobot_cloud_flag_connected();}
        static bool disconnected(void) {return !connected();}
        static void connect(void) {
            intorobot_cloud_flag_connect();
            /*
               if (system_thread_get_state(nullptr)==intorobot::feature::DISABLED &&
               SystemClass::mode() == SEMI_AUTOMATIC) {
            // IntoRobot.connect() should be blocking in SEMI_AUTOMATIC mode when threading is disabled
            waitUntil(connected);
            }
            */
        }
        static void disconnect(void) {intorobot_cloud_flag_disconnect();}
        static void process(void) {
            //application_checkin();
            intorobot_process();
        }
        static int read(void) {
            return intorobot_debug_info_read();
        }
        static int available(void) {
            return intorobot_debug_info_available();
        }
        static String deviceID(void) {return intorobot_deviceID();}
        static void keepAlive(uint16_t sec) {
            intorobot_cloud_keepalive(sec);
        }

    private:
        virtual size_t write(uint8_t byte)
        {return intorobot_debug_info_write(byte);}

        using Print::write; // pull in write(str) and write(buf, size) from Print
};

#endif

#ifndef configNO_LORAWAN
class CloudClass: public CloudDatepointClass {
    public:

        static int connected(void) {
            return LoRaWanActiveStatus();
        }
        static bool disconnected(void) {
            if(LoRaWanActiveStatus() == -1) {
                return true;
            } else {
                return false;
            }
        }
        static int connect(join_mode_t mode, uint16_t timeout) {
            if(mode == JOIN_ABP){
                return LoRaWan.joinABP();
            } else {
                return LoRaWan.joinOTAA(timeout);
            }
        }
        static void disconnect(void) {
            LoRaWanDisconnect();
            LoRaMacAbortRun();
            LoRaWan.setMacClassType(CLASS_A);
        }
        static void setProtocol(protocol_mode_t mode) {
            if(mode == PROTOCOL_LORAWAN) {
                LoRaWanResume();
            } else {
                LoRaWanPause();
            }
        }
        static void process(void) {
            //application_checkin();
            intorobot_process();
        }
        static String deviceID(void) {return intorobot_deviceID();}
};
#endif

#if (!defined configNO_CLOUD) || (!defined configNO_LORAWAN)

CloudClass& __fetch_global_cloud();
#define IntoRobot __fetch_global_cloud()
#define IntoYun __fetch_global_cloud()
#define Cloud __fetch_global_cloud()

#endif

#endif

