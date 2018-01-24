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

#include "intorobot_config.h"

#include "wiring_ticks.h"
#include "system_network.h"
#include "system_network_internal.h"
#include "system_cloud.h"
#include "system_event.h"
#include "system_threading.h"
#include "watchdog_hal.h"
#include "wlan_hal.h"
#include "delay_hal.h"
#include "ui_hal.h"
#include <string.h>

uint32_t wlan_watchdog_base;
uint32_t wlan_watchdog_duration;

volatile uint8_t INTOROBOT_WLAN_RESET;
volatile uint8_t INTOROBOT_WLAN_SLEEP;
volatile uint8_t INTOROBOT_WLAN_STARTED;

#ifndef configNO_NETWORK

#ifdef configWIRING_WIFI_ENABLE
#include "system_network_wifi.h"
WiFiNetworkInterface wifi;
ManagedNetworkInterface& network = wifi;
inline NetworkInterface& nif(network_interface_t _nif) { return wifi; }
#endif

#ifdef configWIRING_CELLULAR_ENABLE
#include "system_network_cellular.h"
CellularNetworkInterface cellular;
ManagedNetworkInterface& network = cellular;
inline NetworkInterface& nif(network_interface_t _nif) { return cellular; }
#endif

void network_notify_connected()
{
    network.notify_connected();
}

void network_notify_disconnected()
{
    network.notify_disconnected();
}

void network_notify_can_shutdown()
{
    network.notify_can_shutdown();
}

void network_notify_dhcp(bool dhcp)
{
    network.notify_dhcp(dhcp);
}

const void* network_config(network_handle_t network, uint32_t param, void* reserved)
{
    return nif(network).config();
}

void network_connect(network_handle_t network, uint32_t flags, uint32_t param, void* reserved)
{
    SYSTEM_THREAD_CONTEXT_ASYNC_CALL(nif(network).connect());
}

void network_disconnect(network_handle_t network, uint32_t param, void* reserved)
{
    SYSTEM_THREAD_CONTEXT_ASYNC_CALL(nif(network).disconnect());
}

bool network_ready(network_handle_t network, uint32_t param, void* reserved)
{
    return nif(network).ready();
}

bool network_connecting(network_handle_t network, uint32_t param, void* reserved)
{
    return nif(network).connecting();
}

/**
 *
 * @param network
 * @param flags    1 - don't change the LED color
 * @param param
 * @param reserved
 */
void network_on(network_handle_t network, uint32_t flags, uint32_t param, void* reserved)
{
    SYSTEM_THREAD_CONTEXT_ASYNC_CALL(nif(network).on());
}

bool network_has_credentials(network_handle_t network, uint32_t param, void* reserved)
{
    SYSTEM_THREAD_CONTEXT_SYNC_CALL_RESULT(nif(network).has_credentials());
}

void network_off(network_handle_t network, uint32_t flags, uint32_t param, void* reserved)
{
    nif(network).connect_cancel(true);
    // flags & 1 means also disconnect the cloud (so it doesn't autmatically connect when network resumed.)
    SYSTEM_THREAD_CONTEXT_ASYNC_CALL(nif(network).off(flags & 1));
}

/**
 *
 * @param network
 * @param flags  bit 0 set means to stop listening.
 * @param
 */
int network_set_credentials(network_handle_t network, uint32_t, NetworkCredentials* credentials, void*)
{
    SYSTEM_THREAD_CONTEXT_SYNC_CALL_RESULT(nif(network).set_credentials(credentials));
}

bool network_clear_credentials(network_handle_t network, uint32_t, NetworkCredentials* creds, void*)
{
    SYSTEM_THREAD_CONTEXT_SYNC_CALL_RESULT(nif(network).clear_credentials());
}

void network_setup(network_handle_t network, uint32_t flags, void* reserved)
{
    SYSTEM_THREAD_CONTEXT_ASYNC_CALL(nif(network).setup());
}

void manage_ip_config()
{
    nif(0).update_config();
}

#endif
