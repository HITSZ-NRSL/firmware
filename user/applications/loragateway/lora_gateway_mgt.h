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

#ifndef LORA_GATEWAY_MGT_H_
#define LORA_GATEWAY_MGT_H_

#include "lora_gateway_config.h"

#define MGT_RESET                0x15 // Not a LoRa Gateway Spec message
#define MGT_SET_SF               0x16
#define MGT_SET_FREQ             0x17

void gateway_mgt(uint8_t size, uint8_t *buff);

#endif

