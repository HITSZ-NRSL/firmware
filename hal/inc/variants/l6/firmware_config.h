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

#ifndef FIRMWARE_CONFIG_H_
#define FIRMWARE_CONFIG_H_

#define FIRMWARE_CONFIG_HAL_USB_CDC 1

#define FIRMWARE_CONFIG_WIRING_LORA 1
#define FIRMWARE_CONFIG_WIRING_USBSERIAL 1

#define FIRMWARE_CONFIG_WIRING_USARTSERIAL 1
#define FIRMWARE_CONFIG_WIRING_USARTSERIAL1 1
#define FIRMWARE_CONFIG_WIRING_USARTSERIAL2 1
#define FIRMWARE_CONFIG_WIRING_WIRE 1
#define FIRMWARE_CONFIG_WIRING_SPI 1
#define FIRMWARE_CONFIG_WIRING_SPI1 1

#define FIRMWARE_CONFIG_SYSTEM_SETUP 1
#define FIRMWARE_CONFIG_SYSTEM_SETUP_USBSERIAL 1
#define FIRMWARE_CONFIG_SYSTEM_SETUP_USARTSERIAL 1
#define FIRMWARE_CONFIG_SYSTEM_LORAWAN 1

#define FIRMWARE_CONFIG_TIM_PWM_FREQ 500 //500Hz
#define FIRMWARE_CONFIG_SERVO_TIM_PWM_FREQ 50  //50Hz

#endif /* FIRMWARE_CONFIG_H */

