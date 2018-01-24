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

#ifndef SYSTEM_SLEEP_H
#define SYSTEM_SLEEP_H

#include <stdint.h>
#include "interrupts_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SLEEP_MODE_NETWORK = 0,
    SLEEP_MODE_DEEP = 1
} system_sleep_mode_t;

void system_sleep(system_sleep_mode_t sleepMode, uint32_t seconds, void* reserved);
void system_sleep_pin(uint16_t wakeUpPin, uint16_t edgeTriggerMode, uint32_t seconds, void(*handler)(), void* reserved);

#ifdef __cplusplus
}
#endif


#endif /* SYSTEM_SLEEP_H */

