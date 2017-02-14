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
#include "wiring_cloud.h"
#include "system_threading.h"
#include "system_product.h"

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
    if(system_mode() == AUTOMATIC)
    {
        network.connect();
    }
    if (network.connected()) {
        system_rgb_blink(RGB_COLOR_BLUE, 1000);//蓝灯闪烁
    }
    else {
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
    }
    else
    {
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
    if (network.connected())
    {
        if (!g_intorobot_cloud_pregrocessed)
        {
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
    if (network.connected())
    {
        if (!g_intorobot_cloud_connected)
        {
            if (in_cloud_backoff_period())
                return;

            int connect_result = intorobot_cloud_connect();
            if (connect_result >= 0)
            {
                g_intorobot_cloud_connected = 1;
                cloud_failed_connection_attempts = 0;
                system_rgb_blink(RGB_COLOR_WHITE, 2000); //白灯闪烁
            }
            else
            {
                g_intorobot_cloud_connected = 0;
                intorobot_cloud_disconnect();
                cloud_connection_failed();
            }
        }
    }
}

void handle_cloud_connection(void)
{
    if (network.connected())
    {
        if (g_intorobot_cloud_connected)
        {
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
    if (intorobot_cloud_flag_auto_connect() == 0)
    {
        intorobot_cloud_disconnect();
    }
    else // cloud connection is wanted
    {
        preprocess_cloud_connection();
        establish_cloud_connection();
        handle_cloud_connection();
    }
}
#endif

void system_process_loop(void)
{
#if PLATFORM_THREADING
    while (1) {
#endif

#ifdef configSETUP_ENABLE
        if(!g_intorobot_system_config)
        {
#endif
            NEWORK_FN(manage_network_connection(), (void)0);
            NEWORK_FN(manage_ip_config(), (void)0);
            CLOUD_FN(manage_cloud_connection(), (void)0);
            CLOUD_FN(manage_app_auto_update(), (void)0);
#ifdef configSETUP_ENABLE
        }
#endif

#if PLATFORM_THREADING
    }
#endif
}

