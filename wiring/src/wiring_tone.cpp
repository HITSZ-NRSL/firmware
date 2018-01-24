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

#include <stdio.h>
#include "wiring_tone.h"
#include "tone_hal.h"

void tone(uint8_t pin, uint32_t frequency, uint32_t duration)
{
    if (pinAvailable(pin) && HAL_Validate_Pin_Function(pin, PF_TIMER)==PF_TIMER) {
        HAL_Tone_Start(pin, frequency, duration);
    }
}

void noTone(uint8_t pin)
{
    if (pinAvailable(pin) && HAL_Validate_Pin_Function(pin, PF_TIMER)==PF_TIMER) {
        HAL_Tone_Stop(pin);
    }
}
