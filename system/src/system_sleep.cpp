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
#include <stdio.h>
#include <stddef.h>
#include "rtc_hal.h"
#include "core_hal.h"
#include "system_sleep.h"
#include "system_network.h"
#include "system_task.h"
#include "system_cloud.h"
#include "system_cloud.h"
#include "system_network_internal.h"
#include "system_threading.h"
#include "system_rgbled.h"
#include "wiring.h"
#include "wiring_system.h"
#include "wiring_interrupts.h"

#ifndef configNO_LORAWAN
static TimerEvent_t sleepTimer;
#endif

static void before_sleep(uint32_t seconds) {
#ifndef configNO_LORAWAN
    LoRa.radioSetSleep();
    if(seconds > 0){
        TimerInit(&sleepTimer, NULL);
        TimerSetValue(&sleepTimer, seconds * 1000);
        TimerStart(&sleepTimer);
    }
#endif
}

static void after_sleep() {
#ifndef configNO_LORAWAN
    TimerStop(&sleepTimer);
    SX1276BoardInit();
    LoRa.radioSetModem(MODEM_LORA);

    if(System.featureEnabled(SYSTEM_FEATURE_LORAMAC_RUN_ENABLED)) {
        if(LoRaWanActiveStatus() == 0) {
            system_rgb_blink(RGB_COLOR_WHITE, 2000); //白灯闪烁
        } else if(INTOROBOT_LORAWAN_JOINED) {
            system_rgb_blink(RGB_COLOR_BLUE, 1000);//蓝灯闪烁
        } else {
            system_rgb_blink(RGB_COLOR_GREEN, 1000);//绿灯闪烁
        }
    } else {
        system_rgb_blink(RGB_COLOR_GREEN, 1000);//绿灯闪烁
    }
#endif
}

int system_sleep_impl(system_sleep_mode_t sleepMode, uint32_t seconds, void* reserved)
{
#ifndef configNO_LORAWAN
    if(LoRaWan.getMacClassType() == CLASS_C){
        return -1;
    }
#endif
    before_sleep(seconds);
    HAL_Core_Enter_Standby_Mode(seconds, nullptr);
    return 0;
}

int system_sleep_pin_impl(uint16_t wakeUpPin, uint16_t edgeTriggerMode, uint32_t seconds, void(*handler)(), void* reserved)
{
#ifndef configNO_LORAWAN
    if(LoRaWan.getMacClassType() == CLASS_C){
        return -1;
    }
#endif
    before_sleep(seconds);
    HAL_Core_Enter_Stop_Mode(wakeUpPin, edgeTriggerMode, seconds);
    after_sleep();
    if(handler != NULL) {
        handler();
    }
    return 0;
}

/**
 * Wraps the actual implementation, which has to return a value as part of the threaded implementation.
 */

void system_sleep(system_sleep_mode_t sleepMode, uint32_t seconds, void* reserved)
{
    system_sleep_impl(sleepMode, seconds, reserved);
}

void system_sleep_pin(uint16_t wakeUpPin, uint16_t edgeTriggerMode, uint32_t seconds, void(*handler)(), void* reserved)
{
    system_sleep_pin_impl(wakeUpPin, edgeTriggerMode, seconds, handler, reserved);
}

