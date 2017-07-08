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
#include "string.h"
#include "watchdog_hal.h"
#include "wlan_hal.h"
#include "delay_hal.h"
#include "timer_hal.h"
#include "core_hal.h"
#include "params_hal.h"
#include "wiring_system.h"
#include "system_task.h"
#include "system_cloud.h"
#include "system_config.h"
#include "system_mode.h"
#include "system_network.h"
#include "system_network_internal.h"
#include "system_update.h"
#include "system_rgbled.h"
#include "intorobot_macros.h"
#include "system_tick_hal.h"
#include "service_debug.h"
#include "wiring_network.h"
#include "wiring_constants.h"
#include "wiring_intorobot.h"
#include "system_threading.h"
#include "system_product.h"
#include "system_lorawan.h"
#include "string_convert.h"
#include "wiring_time.h"

/*debug switch*/
#define SYSTEM_TASK_DEBUG

#ifdef SYSTEM_TASK_DEBUG
#define STASK_DEBUG(...)  do {DEBUG(__VA_ARGS__);}while(0)
#define STASK_DEBUG_D(...)  do {DEBUG_D(__VA_ARGS__);}while(0)
#else
#define STASK_DEBUG(...)
#define STASK_DEBUG_D(...)
#endif

using intorobot::Network;

volatile system_tick_t intorobot_loop_total_millis = 0;
/**
 * Time in millis of the last cloud connection attempt.
 * The next attempt isn't made until the backoff period has elapsed.
 */
static int network_backoff_start = 0;
static int cloud_backoff_start = 0;

/**
 * The number of connection attempts.
 */
static uint8_t network_connection_attempts = 0;
static uint8_t cloud_failed_connection_attempts = 0;

void network_connection_attempt_init()
{
    network_connection_attempts=0;
    network_backoff_start = HAL_Timer_Get_Milli_Seconds();
}

void network_connection_attempted()
{
    if (network_connection_attempts<255)
        network_connection_attempts++;
    network_backoff_start = HAL_Timer_Get_Milli_Seconds();
}

inline uint8_t in_network_backoff_period()
{
    return (HAL_Timer_Get_Milli_Seconds()-network_backoff_start)<backoff_period(network_connection_attempts);
}

void cloud_connection_attempt_init()
{
    cloud_failed_connection_attempts=0;
    cloud_backoff_start = HAL_Timer_Get_Milli_Seconds();
}

void cloud_connection_failed()
{
    if (cloud_failed_connection_attempts<255)
        cloud_failed_connection_attempts++;
    cloud_backoff_start = HAL_Timer_Get_Milli_Seconds();
}

inline uint8_t in_cloud_backoff_period()
{
    return (HAL_Timer_Get_Milli_Seconds()-cloud_backoff_start)<backoff_period(cloud_failed_connection_attempts);
}

#ifndef configNO_NETWORK

void Network_Setup(void)
{
    //系统指示灯, 默认为闪绿灯
    system_rgb_blink(RGB_COLOR_GREEN, 1000);

    network.setup();

    // don't automatically connect when threaded since we want the thread to start asap
    if(system_mode() == AUTOMATIC) {
        network.connect();
    }

    if (network.ready()) {
        //连接上网络，默认蓝灯闪烁
        system_rgb_blink(RGB_COLOR_BLUE, 1000);
    }

    network_connection_attempt_init();
    CLOUD_FN(intorobot_cloud_init(), (void)0);
}

void manage_network_connection()
{
    if (in_network_backoff_period())
        return;

    static bool was_connected = false;
    if (network.ready()) {
        if(!was_connected) {
            system_rgb_blink(RGB_COLOR_BLUE, 1000);//蓝灯闪烁
            INTOROBOT_CLOUD_SOCKETED = 1;
        }
        was_connected = true;
    } else {
        if(was_connected) {
            INTOROBOT_CLOUD_SOCKETED = 0;
#ifndef configNO_CLOUD
            INTOROBOT_CLOUD_CONNECTED = 0;
#endif
            system_rgb_blink(RGB_COLOR_GREEN, 1000);//绿灯闪烁
        }
        was_connected = false;
    }
    network_connection_attempted();
}

#endif

#ifndef configNO_CLOUD
/**
 * terminal auto update
 */
void manage_app_auto_update(void)
{
}

static bool _device_register(void)
{
    //计算签名 signature = md5(timestamp + productSecret)
    MD5Builder md5;
    String payload = "";
    char buffer[33] = {0};

    time_t utc_time = Time.now();
    payload += utc_time;
    system_get_product_secret(buffer, sizeof(buffer));
    payload += buffer;
    md5.begin();
    md5.add((uint8_t *)payload.c_str(), payload.length());
    md5.calculate();
    system_get_product_id(buffer, sizeof(buffer));
    return intorobot_device_register(buffer, md5.toString().c_str());
}

void preprocess_cloud_connection(void)
{
    if (INTOROBOT_CLOUD_SOCKETED) {
        if (!INTOROBOT_CLOUD_CONNECT_PREPARED) {
            if(System.featureEnabled(SYSTEM_FEATURE_AUTO_TIME_SYN_ENABLED)) {
                // 同步时间
                intorobot_sync_time();
            }
            AT_MODE_FLAG_TypeDef at_mode = HAL_PARAMS_Get_System_at_mode();
            //AT_MODE_FLAG_TypeDef at_mode = AT_MODE_FLAG_NONE;
            switch(at_mode)
            {
                case AT_MODE_FLAG_ABP:            //已经灌好密钥
                case AT_MODE_FLAG_OTAA_ACTIVE:    //灌装激活码 已激活
                    break;
                case AT_MODE_FLAG_OTAA_INACTIVE:  //灌装激活码  未激活
                    if(System.featureEnabled(SYSTEM_FEATURE_ACTIVATE_ENABLED)) {
                        // 激活设备成功
                        intorobot_device_activate();
                    }
                    break;
                default:                          //没有密钥信息
                    if(System.featureEnabled(SYSTEM_FEATURE_REGISTER_ENABLED)) {
                        // 注册设备
                        if(_device_register())
                        {
                            HAL_Delay_Milliseconds(200);
                            if(System.featureEnabled(SYSTEM_FEATURE_ACTIVATE_ENABLED)) {
                                // 激活设备
                                intorobot_device_activate();
                            }
                        }
                    }
                    break;
            }
            cloud_connection_attempt_init();
            INTOROBOT_CLOUD_CONNECT_PREPARED = 1;
        }
    }
}

void establish_cloud_connection(void)
{
    if (INTOROBOT_CLOUD_SOCKETED) {
        if (!INTOROBOT_CLOUD_CONNECTED) {
            if (in_cloud_backoff_period())
                return;

            system_notify_event(event_cloud_status, ep_cloud_status_connecting);
            int connect_result = intorobot_cloud_connect();
            if (connect_result >= 0) {
                INTOROBOT_CLOUD_CONNECTED = 1;
                cloud_failed_connection_attempts = 0;
                system_rgb_blink(RGB_COLOR_WHITE, 2000); //白灯闪烁
                system_notify_event(event_cloud_status, ep_cloud_status_connected);
            } else {
                INTOROBOT_CLOUD_CONNECTED = 0;
                intorobot_cloud_disconnect();
                cloud_connection_failed();
                system_notify_event(event_cloud_status, ep_cloud_status_disconnected);
            }
        }
    }
}

void handle_cloud_connection(void)
{
    if (INTOROBOT_CLOUD_SOCKETED) {
        if (INTOROBOT_CLOUD_CONNECTED) {
            int err = intorobot_cloud_handle();
            if (err) {
                INTOROBOT_CLOUD_CONNECTED = 0;
                intorobot_cloud_disconnect();
                system_rgb_blink(RGB_COLOR_BLUE, 1000);
                system_notify_event(event_cloud_status, ep_cloud_status_disconnected);
            }
        }
    }
}

void manage_cloud_connection(void)
{
    preprocess_cloud_connection();
    if (intorobot_cloud_flag_auto_connect() == 0) {
        intorobot_cloud_disconnect();
    } else {
        // cloud connection is wanted
        establish_cloud_connection();
        handle_cloud_connection();
    }
}

void cloud_disconnect(bool closeSocket)
{
    if (INTOROBOT_CLOUD_CONNECTED) {
        system_notify_event(event_cloud_status, ep_cloud_status_disconnecting);
        INTOROBOT_CLOUD_CONNECTED = 0;
        intorobot_cloud_disconnect();
        system_notify_event(event_cloud_status, ep_cloud_status_disconnected);
    }
}

#endif

#ifndef configNO_LORAWAN
void LoraWAN_Setup(void)
{
    if(System.featureEnabled(SYSTEM_FEATURE_LORAMAC_ENABLED))
    {
        STASK_DEBUG("LoraWAN_Setup");
        // Reset the MAC state. Session and pending data transfers will be discarded.
        LoRaWan.begin();

        // AT_MODE_FLAG_TypeDef at_mode = HAL_PARAMS_Get_System_at_mode();
        AT_MODE_FLAG_TypeDef at_mode = AT_MODE_FLAG_OTAA_INACTIVE;
        switch(at_mode)
        {
        case AT_MODE_FLAG_ABP:            //已经灌好密钥
            STASK_DEBUG("AT_MODE_FLAG_ABP");
        case AT_MODE_FLAG_OTAA_ACTIVE:    //灌装激活码 已激活
        {
            STASK_DEBUG("AT_MODE_FLAG_OTAA_ACTIVE");
            char devaddr[16] = {0}, nwkskey[36] = {0}, appskey[36] = {0};
            HAL_PARAMS_Get_System_devaddr(devaddr, sizeof(devaddr));
            HAL_PARAMS_Get_System_nwkskey(nwkskey, sizeof(nwkskey));
            HAL_PARAMS_Get_System_appskey(appskey, sizeof(appskey));
            STASK_DEBUG("devaddr = %s", devaddr);
            STASK_DEBUG("nwkskey = %s", nwkskey);
            STASK_DEBUG("appskey = %s", appskey);

            uint32_t addr = 0;
            uint8_t nwkskeyBuf[16] = {0}, appskeyBuf[16] = {0};
            string2hex(devaddr, (uint8_t *)&addr, 4, true);
            string2hex(nwkskey, nwkskeyBuf, 16, false);
            string2hex(appskey, appskeyBuf, 16, false);

            uint8_t i;
            STASK_DEBUG("dev = 0x%x",addr);

            for( i=0;i<16;i++)
            {
                STASK_DEBUG("nwkSkey= 0x%x",nwkskeyBuf[i]);
            }

            for( i=0;i<16;i++)
            {
                STASK_DEBUG("app skey= 0x%x",appskeyBuf[i]);
            }

            LoRaWan.joinABP(addr,nwkskeyBuf,appskeyBuf);
            INTOROBOT_LORAWAN_JOINED = 1;
            system_rgb_blink(RGB_COLOR_WHITE, 2000); //白灯闪烁
        }
        break;
        case AT_MODE_FLAG_OTAA_INACTIVE:  //灌装激活码  未激活
        {
            STASK_DEBUG("AT_MODE_FLAG_OTAA_INACTIVE");
            system_rgb_blink(RGB_COLOR_GREEN, 1000);//绿灯闪烁
            uint8_t _devEui[8];
            uint8_t _appEui[8];
            uint8_t appKey[16];
            os_getDevEui(_devEui);
            os_getAppEui(_appEui);
            os_getAppKey(appKey);
            uint8_t devEui[8];
            uint8_t appEui[8];

            memcpyr(devEui,_devEui,8);
            memcpyr(appEui,_appEui,8);

            #if 0
            uint8_t i;
            for( i=0;i<8;i++)
            {
                STASK_DEBUG("dev eui= 0x%x",devEui[i]);
            }
            for( i=0;i<8;i++)
            {
                STASK_DEBUG("app eui= 0x%x",appEui[i]);
            }

            for( i=0;i<16;i++)
            {
                STASK_DEBUG("app key= 0x%x",appKey[i]);
            }
            #endif

            STASK_DEBUG("AT_MODE_FLAG_OTAA_INACTIVE");
            LoRaWan.joinOTAA(devEui,appEui,appKey);
        }
        break;
        default:                          //没有密钥信息
            STASK_DEBUG("default");
            system_rgb_blink(RGB_COLOR_GREEN, 1000);//绿灯闪烁
            break;
        }
    }
    else
    {
        LoRaWan.radioInitialize();
    }
}

void manage_lorawan_connection(void)
{
    if(INTOROBOT_LORAWAN_JOINED && !INTOROBOT_LORAWAN_CONNECTED) {
        intorobot_lorawan_send_terminal_info();
        INTOROBOT_LORAWAN_CONNECTED = 1;
    }
    // LoRaWanRunLoop(intorobotParseReceiveDatapoints);
}

#endif

void system_process_loop(void)
{
    intorobot_loop_total_millis = 0;
#ifdef configSETUP_ENABLE
    if(!g_intorobot_system_config) {
#endif
        NEWORK_FN(manage_network_connection(), (void)0);
        NEWORK_FN(manage_ip_config(), (void)0);
        CLOUD_FN(manage_cloud_connection(), (void)0);
        CLOUD_FN(manage_app_auto_update(), (void)0);
        LORAWAN_FN(manage_lorawan_connection(), (void)0);
#ifdef configSETUP_ENABLE
    }
#endif
}

/*
 * @brief This should block for a certain number of milliseconds and also execute system_process_loop
 */
#ifndef configNO_LORAWAN
static void system_delay_pump(unsigned long ms, bool force_no_background_loop=false)
{
    HAL_Core_System_Yield();
    if (ms==0) return;
    system_tick_t intorobot_loop_elapsed_millis = INTOROBOT_LOOP_DELAY_MILLIS;
    intorobot_loop_total_millis += ms;

    system_tick_t start_millis = HAL_Timer_Get_Milli_Seconds();
    system_tick_t end_micros = HAL_Timer_Get_Micro_Seconds() + (1000*ms);

    while (1) {
        HAL_Core_System_Yield();
        system_tick_t elapsed_millis = HAL_Timer_Get_Milli_Seconds() - start_millis;
        if (elapsed_millis > ms) {
            break;
        }
        intorobot_process();
    }
}
#else
static void system_delay_pump(unsigned long ms, bool force_no_background_loop=false)
{
    HAL_Core_System_Yield();
    if (ms==0) return;
    system_tick_t intorobot_loop_elapsed_millis = INTOROBOT_LOOP_DELAY_MILLIS;
    intorobot_loop_total_millis += ms;

    system_tick_t start_millis = HAL_Timer_Get_Milli_Seconds();
    system_tick_t end_micros = HAL_Timer_Get_Micro_Seconds() + (1000*ms);

    while (1) {
        HAL_Core_System_Yield();
        system_tick_t elapsed_millis = HAL_Timer_Get_Milli_Seconds() - start_millis;
        if (elapsed_millis > ms) {
            break;
        } else if (elapsed_millis >= (ms-1)) {
            // on the last millisecond, resolve using millis - we don't know how far in that millisecond had come
            // have to be careful with wrap around since start_micros can be greater than end_micros.
            for (;;) {
                system_tick_t delay = end_micros-HAL_Timer_Get_Micro_Seconds();
                if (delay>100000)
                    return;
                HAL_Delay_Microseconds(min(delay/2, 1u));
            }
        } else {
            HAL_Delay_Milliseconds(1);
        }

        if (INTOROBOT_WLAN_SLEEP || force_no_background_loop) {
            //Do not yield for Spark_Idle()
        } else if ((elapsed_millis >= intorobot_loop_elapsed_millis) \
                || (intorobot_loop_total_millis >= INTOROBOT_LOOP_DELAY_MILLIS)) {
            bool threading = system_thread_get_state(nullptr);
            intorobot_loop_elapsed_millis = elapsed_millis + INTOROBOT_LOOP_DELAY_MILLIS;
            //intorobot_loop_total_millis is reset to 0 in system_process_loop()
            //Run once if the above condition passes
            intorobot_process();
        }
    }
}
#endif

/**
 * On a non threaded platform, or when called from the application thread, then
 * run the background loop so that application events are processed.
 */
void system_delay_ms(unsigned long ms, bool force_no_background_loop=false)
{
    // if not threading, or we are the application thread, then implement delay
    // as a background message pump
    if ((!PLATFORM_THREADING || APPLICATION_THREAD_CURRENT()) && !HAL_IsISR()) {
        system_delay_pump(ms, force_no_background_loop);
    } else {
        HAL_Delay_Milliseconds(ms);
    }
}
