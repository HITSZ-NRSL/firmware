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

#include "intorobot_config.h"

#ifdef configNO_LORAWAN
#define LORAWAN_FN(x,y) (y)
#else
#define LORAWAN_FN(x,y) (x)
#endif

#ifndef configNO_LORAWAN

#include <string.h>
#include <time.h>
#include <stdint.h>
#include "static_assert.h"
#include "wiring_string.h"
#include "wiring_ex_lorawan.h"

#ifdef __cplusplus
extern "C" {
#endif

size_t intorobot_debug_info_write(uint8_t byte);
int intorobot_debug_info_read(void);
int intorobot_debug_info_available(void);
bool intorobot_lorawan_flag_connected(void);
void intorobot_lorawan_send_terminal_info(void);
void intorobot_lorawan_send_data(char* buffer, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif


#endif	/* SYSTEM_LORAWAN_H_ */
