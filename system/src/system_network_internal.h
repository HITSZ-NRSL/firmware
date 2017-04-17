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

#ifndef SYSTEM_NETWORK_INTERNAL_H
#define SYSTEM_NETWORK_INTERNAL_H

#include "intorobot_config.h"
#ifndef configNO_NETWORK

#include "wiring_ticks.h"
#include "system_event.h"
#include "system_network.h"
#include "system_rgbled.h"
#include "system_cloud.h"
#include "system_threading.h"

/*debug switch*/
#define SYSTEM_NETWORK_DEBUG

#ifdef SYSTEM_NETWORK_DEBUG
#define SNETWORK_DEBUG(...)  do {DEBUG(__VA_ARGS__);}while(0)
#define SNETWORK_DEBUG_D(...)  do {DEBUG_D(__VA_ARGS__);}while(0)
#else
#define SNETWORK_DEBUG(...)
#define SNETWORK_DEBUG_D(...)
#endif

enum eWanTimings
{
    CONNECT_TO_ADDRESS_MAX = S2M(30),
    DISCONNECT_TO_RECONNECT = S2M(2),
};

extern volatile uint8_t INTOROBOT_WLAN_RESET;
extern volatile uint8_t INTOROBOT_WLAN_SLEEP;
extern volatile uint8_t INTOROBOT_WLAN_STARTED;

extern uint32_t wlan_watchdog_duration;
extern uint32_t wlan_watchdog_base;

#if defined(DEBUG_WAN_WD)
#define WAN_WD_DEBUG(x,...) DEBUG(x,__VA_ARGS__)
#else
#define WAN_WD_DEBUG(x,...)
#endif


inline void ARM_WLAN_WD(uint32_t x) {
    wlan_watchdog_base = HAL_Timer_Get_Milli_Seconds();
    wlan_watchdog_duration = x;
    WAN_WD_DEBUG("WD Set %d",(x));
}
inline bool WLAN_WD_TO() {
    return wlan_watchdog_duration && ((HAL_Timer_Get_Milli_Seconds()-wlan_watchdog_base)>wlan_watchdog_duration);
}

inline void CLR_WLAN_WD() {
    wlan_watchdog_duration = 0;
    WAN_WD_DEBUG("WD Cleared, was %d",wlan_watchdog_duration);
}

/**
 * Internal network interface class to provide polymorphic behavior for each
 * network type.  This is not part of the dynalib so functions can freely evolve.
 */
struct NetworkInterface
{
    virtual network_interface_t network_interface()=0;
    virtual void setup()=0;

    virtual void on()=0;
    virtual void off(bool disconnect_cloud=false)=0;
    virtual void connect(bool listen_enabled=true)=0;
    virtual bool connecting()=0;
    virtual bool connected()=0;
    virtual void connect_cancel(bool cancel)=0;
    /**
     * Force a manual disconnct.
     */
    virtual void disconnect()=0;

    /**
     * @return {@code true} if this connection was manually taken down.
     */
    virtual bool manual_disconnect()=0;
    /**
     * Perform the 10sec press command, e.g. clear credentials.
     */
    virtual bool ready()=0;

    virtual bool clear_credentials()=0;
    virtual bool has_credentials()=0;
    virtual int set_credentials(NetworkCredentials* creds)=0;

    virtual void config_clear()=0;
    virtual void update_config(bool force=false)=0;
    virtual void* config()=0;       // not really happy about lack of type
};


class ManagedNetworkInterface : public NetworkInterface
{
    volatile uint8_t WLAN_DISCONNECT;
    volatile uint8_t WLAN_DELETE_PROFILES;
    volatile uint8_t WLAN_CONNECTED;
    volatile uint8_t WLAN_CONNECTING;
    volatile uint8_t WLAN_DHCP;
    volatile uint8_t WLAN_CAN_SHUTDOWN;

protected:
    virtual network_interface_t network_interface() override { return 0; }

    virtual void connect_init()=0;
    virtual void connect_finalize()=0;
    virtual void disconnect_now()=0;
    virtual void drive_now()=0;

    virtual void on_now()=0;
    virtual void off_now()=0;

public:

    virtual void get_ipconfig(IPConfig* config)=0;

    virtual void set_error_count(unsigned count)=0;

    bool manual_disconnect() override
    {
        return WLAN_DISCONNECT;
    }

    void set_manual_disconnect(bool disconnect)
    {
        WLAN_DISCONNECT = disconnect;
    }

    bool connected() override
    {
        return WLAN_CONNECTED;
    }

    void connect(bool listen_enabled=true) override
    {
        SNETWORK_DEBUG("ready(): %d; connecting(): %d", (int)ready(), (int)connecting());
        if (!ready() && !connecting()) {
            bool was_sleeping = INTOROBOT_WLAN_SLEEP;

            on(); // activate WiFi

            WLAN_DISCONNECT = 0;
            connect_init();
            INTOROBOT_WLAN_STARTED = 1;
            INTOROBOT_WLAN_SLEEP = 0;

            if (!has_credentials()) {
                if (was_sleeping) {
                    disconnect();
                }
            } else {
                WLAN_CONNECTING = 1;
                SNETWORK_DEBUG("ARM_WLAN_WD 1");
                ARM_WLAN_WD(CONNECT_TO_ADDRESS_MAX);    // reset the network if it doesn't connect within the timeout
                connect_finalize();
            }
        }
    }

    void disconnect() override
    {
        if (INTOROBOT_WLAN_STARTED) {
            WLAN_DISCONNECT = 1; //Do not ARM_WLAN_WD() in WLAN_Async_Callback()
            WLAN_CONNECTING = 0;
            WLAN_CONNECTED = 0;
            WLAN_DHCP = 0;

            CLOUD_FN(cloud_disconnect(), (void)0);
            disconnect_now();
            config_clear();
        }
    }

    bool ready() override
    {
        drive_now();
        return (INTOROBOT_WLAN_STARTED && WLAN_DHCP);
    }

    bool connecting() override
    {
        drive_now();
        return (INTOROBOT_WLAN_STARTED && WLAN_CONNECTING);
    }

    void on() override
    {
        if (!INTOROBOT_WLAN_STARTED) {
            config_clear();
            on_now();
            update_config(true);
            INTOROBOT_WLAN_STARTED = 1;
            INTOROBOT_WLAN_SLEEP = 0;
        }
    }

    void off(bool disconnect_cloud=false) override
    {
        if (INTOROBOT_WLAN_STARTED) {
            disconnect();
            off_now();

            INTOROBOT_WLAN_SLEEP = 1;

            if (disconnect_cloud) {
                CLOUD_FN(intorobot_cloud_flag_disconnect(), (void)0);
            }

            INTOROBOT_WLAN_STARTED = 0;
            WLAN_DHCP = 0;
            WLAN_CONNECTED = 0;
            WLAN_CONNECTING = 0;
        }
    }

    void notify_connected()
    {
        WLAN_CONNECTED = 1;
        WLAN_CONNECTING = 0;

        /* If DHCP has completed, don't re-arm WD due to spurious notify_connected()
         * from WICED on loss of internet and reconnect
         */
        if (!WLAN_DISCONNECT && !WLAN_DHCP) {
            SNETWORK_DEBUG("ARM_WLAN_WD 2");
            ARM_WLAN_WD(CONNECT_TO_ADDRESS_MAX);
        }
    }

    void notify_disconnected()
    {
        CLOUD_FN(cloud_disconnect(false), (void)0);
        if (WLAN_CONNECTED) {
            //Breathe blue if established connection gets disconnected
            if (!WLAN_DISCONNECT) {
                //if WiFi.disconnect called, do not enable wlan watchdog
                SNETWORK_DEBUG("ARM_WLAN_WD 3");
                ARM_WLAN_WD(DISCONNECT_TO_RECONNECT);
            }
        }
        WLAN_CONNECTED = 0;
        WLAN_CONNECTING = 0;
        WLAN_DHCP = 0;
    }

    void notify_dhcp(bool dhcp)
    {
        WLAN_CONNECTING = 0;
        if (dhcp) {
            SNETWORK_DEBUG("CLR_WLAN_WD 1, DHCP success");
            CLR_WLAN_WD();
            WLAN_DHCP = 1;
        } else {
            config_clear();
            WLAN_DHCP = 0;
            SNETWORK_DEBUG("DHCP fail, ARM_WLAN_WD 5");
            ARM_WLAN_WD(DISCONNECT_TO_RECONNECT);
        }
    }

    void notify_can_shutdown()
    {
        WLAN_CAN_SHUTDOWN = 1;
    }

    void notify_cannot_shutdown()
    {
        WLAN_CAN_SHUTDOWN = 0;
    }

    inline bool hasDHCP()
    {
        return WLAN_DHCP && !INTOROBOT_WLAN_SLEEP;
    }
};

extern ManagedNetworkInterface& network;

template <typename Config, typename C>
class ManagedIPNetworkInterface : public ManagedNetworkInterface
{
    Config ip_config;

public:
    void get_ipconfig(IPConfig* config) override
    {
        update_config(true);
        memcpy(config, this->config(), config->size);
    }

    void update_config(bool force=false) override
    {
        // todo - IPv6 may not set this field.
        bool fetched_config = ip_config.nw.aucIP.ipv4!=0;
        if (hasDHCP() || force) {
            if (!fetched_config || force) {
                memset(&ip_config, 0, sizeof(ip_config));
                ip_config.size = sizeof(ip_config);
                reinterpret_cast<C*>(this)->fetch_ipconfig(&ip_config);
            }
        } else if (fetched_config) {
            config_clear();
        }
    }

    void config_clear() override
    {
        memset(&ip_config, 0, sizeof(ip_config));
    }

    void* config() override  { return &ip_config; }

};

#endif

#endif/* SYSTEM_NETWORK_INTERNAL_H */
