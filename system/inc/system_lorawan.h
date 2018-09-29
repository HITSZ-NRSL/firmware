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

#ifndef SYSTEM_LORAWAN_H_
#define SYSTEM_LORAWAN_H_

#include "firmware_platform_config.h"

#if FIRMWARE_CONFIG_SYSTEM_LORAWAN
#define LORAWAN_FN(x,y) (x)
#else
#define LORAWAN_FN(x,y) (y)
#endif

#if FIRMWARE_CONFIG_SYSTEM_LORAWAN

#include <string.h>
#include <time.h>
#include <stdint.h>
#include "static_assert.h"
#include "wiring_string.h"
#include "wiring_lorawan.h"
#include "system_event.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum event_t{
    LORAWAN_EVENT_JOINED,                   //已入网
    LORAWAN_EVENT_JOIN_FAIL,                //入网失败
    LORAWAN_EVENT_RX_COMPLETE,              //接收完成
    LORAWAN_EVENT_MCPSINDICATION_CONFIRMED, //收到服务器确认帧
}lorawan_event_t;


size_t intorobot_debug_info_write(uint8_t byte);
int intorobot_debug_info_read(void);
int intorobot_debug_info_available(void);
bool intorobot_lorawan_flag_connected(void);
void intorobot_lorawan_send_terminal_info(void);
int intorobot_lorawan_send_data(char* buffer, uint16_t len, bool confirmed, uint16_t timeout);
void LoRaWanPause(void);
void LoRaWanResume(void);
void LoRaWanOnEvent(lorawan_event_t event);
void LoRaWanJoinEnable(bool enable);
bool LoRaWanJoinIsEnabled(void);
int  LoRaWanActiveStatus(void);
void LoRaWanJoinOTAA(void);
bool LoRaWanJoinABP(void);
void LoRaWanDisconnect(void);
void os_getDevEui(uint8_t *buf);
void os_getAppEui(uint8_t *buf);
void os_getAppKey(uint8_t *buf);

#ifdef __cplusplus
}
#endif

#endif


#endif	/* SYSTEM_LORAWAN_H_ */
