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

#ifndef INTOROBOT_CONFIG_H_
#define INTOROBOT_CONFIG_H_

/* config hal function. */
#define configHAL_USB_CDC_ENABLE
//#define configHAL_USB_HID_ENABLE

/* config wiring communication type. */
#define configWIRING_WIFI_ENABLE
//#define configWIRING_CELLULAR_ENABLE
//#define configWIRING_LORA_ENABLE
//#define configWIRING_IPV6_ENABLE

/* config wiring usb serial. */
#define configWIRING_USBSERIAL_ENABLE
//#define configWIRING_USBMOUSE_ENABLE
//#define configWIRING_USBKEYBOARD_ENABLE

/* config wiring usart serial. */
#define configWIRING_USARTSERIAL_ENABLE
//#define configWIRING_USARTSERIAL1_ENABLE
//#define configWIRING_USARTSERIAL2_ENABLE
//#define configWIRING_USARTSERIAL3_ENABLE
//#define configWIRING_USARTSERIAL4_ENABLE
//#define configWIRING_USARTSERIAL5_ENABLE

/* config wring wire */
#define configWIRING_WIRE_ENABLE
//#define configWIRING_WIRE1_ENABLE
//#define configWIRING_WIRE2_ENABLE

/* config wring I2S */
//#define configWIRING_I2S_ENABLE
//#define configWIRING_I2S1_ENABLE
//#define configWIRING_SPI2_ENABLE

/* config wring spi */
#define configWIRING_SPI_ENABLE
#define configWIRING_SPI1_ENABLE
//#define configWIRING_SPI2_ENABLE

/* config wiring can. */
//#define configWIRING_CAN_ENABLE
//#define configWIRING_CAN1_ENABLE

/* config setup */
#define configSETUP_ENABLE
#define configSETUP_USBSERIAL_ENABLE
//#define configSETUP_USARTSERIAL_ENABLE
//#define configSETUP_TCP_ENABLE
#define configSETUP_UDP_ENABLE

/* config network */
//#define configNO_NETWORK

/* config cloud */
//#define configNO_CLOUD

/* config cloud */
#define configNO_LORAWAN

/* config rgb ui */
//#define configNO_RGB_UI

/* config setup button ui */
//#define configNO_SETUPBUTTON_UI

/* config panic */
#define configPANIC_BUT_KEEP_CALM


#define configTIM_PWM_FREQ                      500 //500Hz
#define configSERVO_TIM_PWM_FREQ                50  //50Hz


#endif /* INTOROBOT_CONFIG_H */

