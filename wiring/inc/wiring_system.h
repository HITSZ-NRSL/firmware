/**
 ******************************************************************************
  Copyright (c) 2013-2015 IntoRobot Industries, Inc.  All rights reserved.

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


#ifndef WIRING_SYSTEM_H__
#define WIRING_SYSTEM_H__

#include "wiring_ticks.h"
#include "wiring_string.h"
#include "wiring_product.h"
#include "system_mode.h"
#include "system_update.h"
#include "system_sleep.h"
#include "system_cloud.h"
#include "system_event.h"
#include "system_utilities.h"
#include "core_hal.h"
#include "interrupts_hal.h"
#include "core_hal.h"
#include "system_user.h"
#include "system_config.h"
#include "system_task.h"
#include "wiring_time.h"

#ifdef INTOROBOT_PLATFORM
#define SYSTEM_HW_TICKS 1
#else
#define SYSTEM_HW_TICKS 0
#endif

#if SYSTEM_HW_TICKS
#include "hw_ticks.h"
#endif

class Stream;

class SystemClass {
    public:

        SystemClass(System_Mode_TypeDef mode = DEFAULT) {
            set_system_mode(mode);
        }

        static System_Mode_TypeDef mode(void) {
            return system_mode();
        }

        static bool firmwareUpdate(Stream *serialObj) {
            //return system_firmwareUpdate(serialObj);
            return true;
        }

        static void factoryReset(void);
        static void dfu(bool persist=false);
        static void reset(void);

        static void onSysTick(void(*handler)(void)) {
            SetSysTickHandler(handler);
        }

#if SYSTEM_HW_TICKS
        static inline uint32_t ticksPerMicrosecond() {
            return SYSTEM_US_TICKS;
        }
        static inline uint32_t ticks() {
            return SYSTEM_TICK_COUNTER;
        }
        static inline void ticksDelay(uint32_t duration) {
            uint32_t start = ticks();
            while ((ticks()-start)<duration) {}
        }
#endif
        /**
         * @brief 设备进入STOP睡眠模式(唤醒后重启)或者使通讯模组进入睡眠模式
         * @param sleepMode 睡眠模式类型
         * @param seconds 睡眠时间
         * @return
         */
        static void sleep(system_sleep_mode_t sleepMode, long seconds=0);

        /**
         * @brief 使通讯模组进入睡眠模式
         * @param seconds 睡眠时间
         * @return
         */
        inline static void sleep(long seconds) {
            sleep(SLEEP_MODE_NETWORK, seconds);
        }

        /**
         * @brief 设备进入stop睡眠模式(唤醒后继续执行)
         * @param wakeUpPin 唤醒引脚
         * @param edgeTriggerMode 引脚唤醒方式
         * @param seconds 睡眠时间
         * @param handler 唤醒回调函数
         * @return
         */
        static void sleep(uint16_t wakeUpPin, InterruptMode edgeTriggerMode, uint32_t seconds=0, void(*handler)()=NULL);

        /**
         * @brief 设备进入stop睡眠模式(唤醒后继续执行)
         * @param handler 唤醒回调函数
         * @param seconds 睡眠时间
         * * @return
         */
        inline static void sleep(void(*handler)(), uint32_t seconds=0) __attribute__((deprecated("Please use WiFi.connect() instead"))) {
            sleep(0xff, CHANGE, seconds, handler);
        }

        static void process(void) { intorobot_process(); }

        static String deviceID(void) { return intorobot_deviceID(); }
        static uint16_t buttonPushed(uint8_t button=0) {
            return system_button_pushed_duration(button, NULL);
        }
        //system event
        static bool on(system_event_t events, void(*handler)(system_event_t event, int param, uint8_t *data, uint16_t datalen)) {
            return !system_subscribe_event(events, reinterpret_cast<system_event_handler_t*>(handler), nullptr);
        }
        static bool on(system_event_t events, void(*handler)(system_event_t event, int param)) {
            return system_subscribe_event(events, reinterpret_cast<system_event_handler_t*>(handler), NULL);
        }
        static bool on(system_event_t events, void(*handler)(system_event_t event)) {
            return system_subscribe_event(events, reinterpret_cast<system_event_handler_t*>(handler), NULL);
        }
        static bool on(system_event_t events, void(*handler)()) {
            return system_subscribe_event(events, reinterpret_cast<system_event_handler_t*>(handler), NULL);
        }
        static void off(void(*handler)(system_event_t event, int param, uint8_t *data, uint16_t datalen)) {
            system_unsubscribe_event(event_all, handler, nullptr);
        }
        static void off(system_event_t events, void(*handler)(system_event_t event, int param, uint8_t *data, uint16_t datalen)) {
            system_unsubscribe_event(events, handler, nullptr);
        }
        static uint32_t freeMemory();
        template<typename Condition, typename While> static bool waitConditionWhile(Condition _condition, While _while) {
            while (_while() && !_condition()) {
                intorobot_process();
            }
            return _condition();
        }
        template<typename Condition> static bool waitCondition(Condition _condition) {
            return waitConditionWhile(_condition, []{ return true; });
        }
        template<typename Condition> static bool waitCondition(Condition _condition, system_tick_t timeout) {
            const system_tick_t start = millis();
            return waitConditionWhile(_condition, [=]{ return (millis()-start)<timeout; });
        }
        bool get(system_params_t params_type, int &value) {
            return system_get_params_int(params_type, value) >= 0;
        }
        bool get(system_params_t params_type, double &value) {
            return system_get_params_double(params_type, value) >= 0;
        }
        uint16_t get(system_params_t params_type, const void *data, uint16_t length) {
            return system_get_params_array(params_type, (char *)data, length);
        }
        bool set(system_params_t params_type, int value) {
            return system_set_params_int(params_type, value) >= 0;
        }
        bool set(system_params_t params_type, double value) {
            return system_set_params_double(params_type, value) >= 0;
        }
        bool set(system_params_t params_type, const char *data) {
            return set(params_type, data, strlen(data));
        }
        bool set(system_params_t params_type, const void *data, uint16_t length) {
            return system_set_params_array(params_type, (char *)data, length) >= 0;
        }
        inline bool featureEnabled(system_feature_t feature) {
            return system_product_instance().get_system_feature(feature);
        }
        inline int enableFeature(system_feature_t feature) {
            return system_product_instance().set_system_feature(feature, true);
        }
        inline int disableFeature(system_feature_t feature) {
            return system_product_instance().set_system_feature(feature, false);
        }
#ifdef configSETUP_ENABLE
        void configEnterMode(system_config_type_t config_type) {
            set_system_config_type(config_type);
        }
        void configExit(void) {
            set_system_config_type(SYSTEM_CONFIG_TYPE_NONE);
        }
        system_config_type_t configCurrentMode(void) {
            return get_system_config_type();
        }
        void configProcess(void) {
            system_config_process();
        }
#endif
        String version(void) {
            char version[32] = {0};

            system_get_firmlib_version(version, sizeof(version));
            return String(version);
        }
#ifndef configNO_CLOUD
        bool deviceRegister(char *prodcut_id, size_t utc_time, char *signature) {
            return intorobot_device_register(prodcut_id, utc_time, signature);
        }
        void setJoinParams(char *deviceID, char *accessToken) {
            HAL_PARAMS_Set_System_device_id(deviceID);
            HAL_PARAMS_Set_System_access_token(accessToken);
            HAL_PARAMS_Set_System_at_mode(AT_MODE_FLAG_ABP);
            HAL_PARAMS_Save_Params();
        }
        void setBasicParams(float zone, char *serverDomain, int serverPort, char *registerDomain, int registerPort, char *updateDomain) {
            HAL_PARAMS_Set_System_zone(zone);
            HAL_PARAMS_Set_System_sv_domain(serverDomain);
            HAL_PARAMS_Set_System_sv_port(serverPort);
            HAL_PARAMS_Set_System_http_domain(registerDomain);
            HAL_PARAMS_Set_System_http_port(registerPort);
            HAL_PARAMS_Set_System_dw_domain(updateDomain);
            HAL_PARAMS_Save_Params();
            Time.zone(zone);
        }
        float getTimeZone(void) {
            return HAL_PARAMS_Get_System_zone();
        }
        uint16_t getServerDomain(char *domain, uint16_t len) {
            return HAL_PARAMS_Get_System_sv_domain(domain, len);
        }
        int getServerPort(void) {
            return HAL_PARAMS_Get_System_sv_port();
        }
        uint16_t getRegisterDomain(char *domain, uint16_t len) {
            return HAL_PARAMS_Get_System_http_domain(domain, len);
        }
        int getRegisterPort(void) {
            return HAL_PARAMS_Get_System_http_port();
        }
        uint16_t getUpdateDomain(char *domain, uint16_t len) {
            return HAL_PARAMS_Get_System_dw_domain(domain, len);
        }
#endif
};

extern SystemClass System;

#define SYSTEM_MODE(mode)  SystemClass SystemMode(mode);
#define SYSTEM_THREAD(state) STARTUP(system_thread_set_state(intorobot::feature::state, NULL));

#define waitFor(condition, timeout) System.waitCondition([]{ return (condition)(); }, (timeout))
#define waitUntil(condition) System.waitCondition([]{ return (condition)(); })

#endif  /* WIRING_SYSTEM_H__ */

