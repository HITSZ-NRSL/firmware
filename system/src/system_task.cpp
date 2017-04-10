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
    network.setup();

    // don't automatically connect when threaded since we want the thread to start asap
    if(system_mode() == AUTOMATIC) {
        network.connect();
    }
    if (network.connected()) {
        system_rgb_blink(RGB_COLOR_BLUE, 1000);//蓝灯闪烁
    } else {
        system_rgb_blink(RGB_COLOR_GREEN, 1000);//绿灯闪烁
    }
    network_connection_attempt_init();
    CLOUD_FN(intorobot_cloud_init(), (void)0);
}

void manage_network_connection()
{
    if (in_network_backoff_period())
        return;

    static bool was_connected = false;
    if (network.status()) {
        if(!was_connected) {
            system_rgb_blink(RGB_COLOR_BLUE, 1000);//蓝灯闪烁
        }
        was_connected = true;
    } else {
        if(was_connected) {
#ifndef configNO_CLOUD
            g_intorobot_cloud_connected = 0;
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
#if 0
    if (network.connected())
    {
        if (g_intorobot_cloud_connected)
        {
            //发送时间间隔到
            static system_tick_t start_millis = millis();
            system_tick_t current_millis = millis();
            system_tick_t elapsed_millis = current_millis - start_millis;
            if (elapsed_millis < 0)
            {
                elapsed_millis =  0xFFFFFFFF - start_millis + current_millis;
            }
            if (elapsed_millis < 300*1000)  //5分钟查询一次
            {
                return;
            }
            start_millis = millis();

            String body;
            if(intorobot_get_version(body))
            {
                aJsonClass aJson;
                aJsonObject *root = aJson.parse(body.c_str());
                if (root == NULL)
                {return false;}

                //获取product id
                product_details_t product_details;
                system_product_instance().get_product_details(product_details);
                String version = product_details.product_firmware_version;

                //固件升级
                aJsonObject *firmwareVerObject = aJson.getObjectItem(root, "firmwareVer");
                if ( firmwareVerObject != NULL ) {
                    //固件判断升级
                    if((strcmp(firmwareVerObject->valuestring,""))&&(strcmp(firmwareVerObject->valuestring,version.c_str())))
                    {
                        //固件升级
                    }
                    return true;
                }
                //子系统升级
                aJsonObject *subsysVerObject = aJson.getObjectItem(root, "subsysVer");
                if ( subsysVerObject != NULL ) {
                    //子系统版本判断升级
                }
                aJson.deleteItem(root);
            }
        }
    }
#endif
}

void preprocess_cloud_connection(void)
{
    if (network.connected()) {
        if (!g_intorobot_cloud_pregrocessed) {

            // 同步时间
            intorobot_sync_time();

            AT_MODE_FLAG_TypeDef at_mode = HAL_PARAMS_Get_System_at_mode();
            //AT_MODE_FLAG_TypeDef at_mode = AT_MODE_FLAG_NONE;
            switch(at_mode)
            {
                case AT_MODE_FLAG_ABP:            //已经灌好密钥
                case AT_MODE_FLAG_OTAA_ACTIVE:    //灌装激活码 已激活
                    break;
                case AT_MODE_FLAG_OTAA_INACTIVE:  //灌装激活码  未激活
                    // 激活设备成功
                    intorobot_device_activate();
                    break;
                default:                          //没有密钥信息
                    // 注册设备
                    if(intorobot_device_register())
                    {
                        // 激活设备
                        intorobot_device_activate();
                    }
                    break;
            }
            cloud_connection_attempt_init();
            g_intorobot_cloud_pregrocessed = 1;
        }
    }
}

void establish_cloud_connection(void)
{
    if (network.connected()) {
        if (!g_intorobot_cloud_connected) {
            if (in_cloud_backoff_period())
                return;

            int connect_result = intorobot_cloud_connect();
            if (connect_result >= 0) {
                g_intorobot_cloud_connected = 1;
                cloud_failed_connection_attempts = 0;
                system_rgb_blink(RGB_COLOR_WHITE, 2000); //白灯闪烁
            } else {
                g_intorobot_cloud_connected = 0;
                intorobot_cloud_disconnect();
                cloud_connection_failed();
            }
        }
    }
}

void handle_cloud_connection(void)
{
    if (network.connected()) {
        if (g_intorobot_cloud_connected) {
            int err = intorobot_cloud_handle();
            if (err) {
                g_intorobot_cloud_connected = 0;
                intorobot_cloud_disconnect();
                system_rgb_blink(RGB_COLOR_BLUE, 1000);
            }
        }
    }
}

void manage_cloud_connection(void)
{
    if (intorobot_cloud_flag_auto_connect() == 0) {
        intorobot_cloud_disconnect();
    } else {
        // cloud connection is wanted
        preprocess_cloud_connection();
        establish_cloud_connection();
        handle_cloud_connection();
    }
}
#endif

#ifndef configNO_LORAWAN
void LoraWAN_Setup(void)
{
    STASK_DEBUG("LoraWAN_Setup");
    // LMIC init
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    AT_MODE_FLAG_TypeDef at_mode = HAL_PARAMS_Get_System_at_mode();
    //AT_MODE_FLAG_TypeDef at_mode = AT_MODE_FLAG_OTAA_INACTIVE;
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
                LMIC_setSession (0x1, addr, nwkskeyBuf, appskeyBuf);
                LMIC_setLinkCheckMode(0);
                LMIC.dn2Dr = DR_SF9;
                LMIC_setDrTxpow(DR_SF7,14);
                g_intorobot_lorawan_joined = 1;
                system_rgb_blink(RGB_COLOR_WHITE, 2000); //白灯闪烁
            }
            break;
        case AT_MODE_FLAG_OTAA_INACTIVE:  //灌装激活码  未激活
            STASK_DEBUG("AT_MODE_FLAG_OTAA_INACTIVE");
            system_rgb_blink(RGB_COLOR_GREEN, 1000);//绿灯闪烁
            LMIC_startJoining();
            break;
        default:                          //没有密钥信息
            STASK_DEBUG("default");
            system_rgb_blink(RGB_COLOR_GREEN, 1000);//绿灯闪烁
            break;
    }
}

void manage_lorawan_connection(void)
{
    if(g_intorobot_lorawan_joined && !g_intorobot_lorawan_connected) {
        intorobot_lorawan_send_terminal_info();
        g_intorobot_lorawan_connected = 1;
    }
    os_runloop_once();
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
    if (ms==0) return;
    system_tick_t intorobot_loop_elapsed_millis = INTOROBOT_LOOP_DELAY_MILLIS;
    intorobot_loop_total_millis += ms;

    system_tick_t start_millis = HAL_Timer_Get_Milli_Seconds();
    system_tick_t end_micros = HAL_Timer_Get_Micro_Seconds() + (1000*ms);

    while (1) {
        system_tick_t elapsed_millis = HAL_Timer_Get_Milli_Seconds() - start_millis;
        if (elapsed_millis > ms) {
            break;
        }
        if(!intorobot_process_flag) {
            intorobot_process();
        }
    }
}
#else
static void system_delay_pump(unsigned long ms, bool force_no_background_loop=false)
{
    if (ms==0) return;
    system_tick_t intorobot_loop_elapsed_millis = INTOROBOT_LOOP_DELAY_MILLIS;
    intorobot_loop_total_millis += ms;

    system_tick_t start_millis = HAL_Timer_Get_Milli_Seconds();
    system_tick_t end_micros = HAL_Timer_Get_Micro_Seconds() + (1000*ms);

    while (1) {
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
            if(!intorobot_process_flag) {
                intorobot_process();
            }
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

