/**
  ******************************************************************************
  * @file    application.h
  * @authors  Satish Nair, Zachary Crockett, Zach Supalla and Mohit Bhoite
  * @version V1.0.0
  * @date    30-April-2013
  * @brief   User Application File Header
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

#ifndef APPLICATION_H_
#define APPLICATION_H_

#ifndef INTOROBOT_WIRING_ARDUINO_COMPATIBILTY
#define INTOROBOT_WIRING_ARDUINO_COMPATIBILTY 0
#endif

#include "stdio.h"

#ifdef INTOROBOT_PLATFORM
#include "platform_headers.h"
#endif
#include "platforms.h"
#include "system_utilities.h"

#include "wiring.h"
#include "wiring_log.h"
#include "wiring_cloud.h"
#include "wiring_interrupts.h"
#include "wiring_string.h"
//#include "wiring_power.h"
//#include "wiring_fuel.h"
#include "wiring_print.h"
#include "wiring_usartserial.h"
#include "wiring_can.h"
#include "wiring_usbserial.h"
#include "wiring_usbmouse.h"
#include "wiring_usbkeyboard.h"
#include "wiring_spi.h"
#include "wiring_i2c.h"
#include "wiring_i2s.h"
#include "wiring_servo.h"
#include "wiring_wifi.h"
#include "wiring_network.h"
#include "wiring_client.h"
#include "wiring_startup.h"
#include "wiring_timer.h"
#include "wiring_tcpclient.h"
#include "wiring_tcpserver.h"
#include "wiring_httpclient.h"
#include "wiring_udp.h"
#include "wiring_time.h"
#include "wiring_tone.h"
#include "wiring_eeprom.h"
#include "wiring_product.h"
#include "wiring_watchdog.h"
#include "wiring_thread.h"
#include "wiring_system.h"
#include "wiring_ajson.h"
#include "wiring_mqttclient.h"
#include "fast_pin.h"
#include "string_convert.h"

// this was being implicitly pulled in by some of the other headers
// adding here for backwards compatibility.
#include "system_task.h"
#include "system_user.h"

#include "wiring_ex.h"

using namespace intorobot;

#include "wiring_arduino.h"
#endif /* APPLICATION_H_ */

