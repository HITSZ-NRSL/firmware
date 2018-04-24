/*
 ******************************************************************************
  Copyright (c) 2015 IntoRobot Industries, Inc.  All rights reserved.

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

#pragma once

#include <cstdint>
#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t system_event_t;
typedef void (system_event_handler_t)(system_event_t event, int param, uint8_t *data, uint16_t datalen);

enum SystemEvents {
    event_mode_changed           = 1<<1,         // mode event
    event_network_status         = 1<<2,         // network event
    event_cloud_status           = 1<<3,         // cloud event
    event_app_status             = 1<<4,         // app event
    event_cloud_data             = 1<<5,         // cloud data event
    event_reset                  = 1<<6,         // notifies that the system will now reset on return from this event.
    event_time_changed           = 1<<7,
    event_low_battery            = 1<<8,         // generated when low battery condition is detected
    event_lora_radio_status      = 1<<9,         //lora radio event
    event_lorawan_status         = 1<<10,        //lorawan event

    event_all                    = 0xFFFFFFFFFFFFFFFF
};

enum SystemEventsParam {
    //mode change
    ep_mode_normal                     = 1,
    ep_mode_imlink_config              = 2,
    ep_mode_ap_config                  = 3,
    ep_mode_imlink_serial_config       = 4,
    ep_mode_ap_serial_config           = 5,
    ep_mode_serial_config              = 6,
    ep_mode_binding                    = 7,
    ep_mode_test                       = 8,

    //network status
    ep_network_status_powering_off     = 1,
    ep_network_status_off              = 2,
    ep_network_status_powering_on      = 3,
    ep_network_status_on               = 4,
    ep_network_status_connecting       = 5,
    ep_network_status_connected        = 6,
    ep_network_status_disconnecting    = 7,
    ep_network_status_disconnected     = 8,

    // Cloud connection status
    ep_cloud_status_disconnected       = 1,
    ep_cloud_status_connecting         = 2,
    ep_cloud_status_connected          = 3,
    ep_cloud_status_disconnecting      = 4,

    // app change status
    ep_app_status_disconneted          = 1,
    ep_app_status_connected            = 2,

    // cloud data 替换成cloud comm 为了兼容之前程序保留
    ep_cloud_data_raw                  = 1,  //原始数据 事件
    ep_cloud_data_datapoint            = 2,  //数据点数据协议处理 事件
    ep_cloud_data_custom               = 3,  //自定义数据协议处理 事件

    // cloud comm
    ep_cloud_comm_data                 = 1,  //接收到云端数据
    ep_cloud_comm_ota                  = 2,  //接收到云端OTA升级事件

    // time change
    ep_time_changed_manually           = 1,
    ep_time_changed_sync               = 2,

    //lorawan radio
    ep_lora_radio_tx_done              = 1,
    ep_lora_radio_tx_fail              = 2,
    ep_lora_radio_rx_done              = 3,
    ep_lora_radio_rx_timeout           = 4,
    ep_lora_radio_rx_error             = 5,
    ep_lora_radio_cad_done             = 6,
    ep_lora_radio_cad_detected         = 7,

    //lorawan
    ep_lorawan_join_success             = 1,
    ep_lorawan_join_fail                = 2,
    ep_lorawan_send_success             = 3,
    ep_lorawan_send_fail                = 4,
    ep_lorawan_mcpsconfirm_confirmed    = 5,
    ep_lorawan_mcpsconfirm_unconfirmed  = 6,
};

/**
 * Subscribes to the system events given
 * @param events    One or more system events. Multiple system events are specified using the + operator.
 * @param handler   The system handler function to call.
 * @param reserved  Set to NULL.
 * @return {@code 0} if the system event handlers were registered successfully. Non-zero otherwise.
 */
int system_subscribe_event(system_event_t events, system_event_handler_t* handler, void* reserved);

/**
 * Unsubscribes a handler from the given events.
 * @param handler   The handler that will be unsubscribed.
 * @param reserved  Set to NULL.
 */
void system_unsubscribe_event(system_event_t events, system_event_handler_t* handler, void* reserved);

#ifdef __cplusplus
}
#endif

/**
 * Notifies all subscribers about an event.
 * @param event
 * @param data
 * @param pointer
 */
void system_notify_event(system_event_t event, int param=0, uint8_t *data=NULL, uint16_t datalen=0, void (*fn)(void* data)=nullptr, void* fndata=nullptr);

