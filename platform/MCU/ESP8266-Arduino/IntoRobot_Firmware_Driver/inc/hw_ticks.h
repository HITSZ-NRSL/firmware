/**
  Copyright (c) 2015 IntoRobot Industries, Inc.  All rights reserved.

  Copyright 2012 STMicroelectronics
  http://www.st.com/software_license_agreement_liberty_v2

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

#pragma once

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

extern uint32_t xthal_get_ccount();

typedef uint32_t system_tick_t;

#define SYSTEM_US_TICKS         ( F_CPU / 1000000L )    //cycles per microsecond
#define SYSTEM_TICK_COUNTER     xthal_get_ccount()

#define clockCyclesPerMicrosecond() SYSTEM_US_TICKS
#define clockCyclesToMicroseconds(a) ( (a) / clockCyclesPerMicrosecond() )
#define microsecondsToClockCycles(a) ( (a) * clockCyclesPerMicrosecond() )

void SysTick_Enable();
/**
 * Fetch the current milliseconds count.
 * @return the number of milliseconds since the device was powered on or woken up from
 * sleep. Automatically wraps around when above UINT_MAX;
 */
system_tick_t GetSystem1MsTick();

/**
 * Fetch the current microseconds count.
 * @return the number of microseconds since the device was powered on or woken up from
 * sleep. Automatically wraps around when above UINT_MAX;
 */
system_tick_t GetSystem1UsTick();



#ifdef	__cplusplus
}
#endif



