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

/* Includes ------------------------------------------------------------------*/
#include "tone_hal.h"

void HAL_Tone_Start(uint8_t pin, uint32_t frequency, uint32_t duration)
{
}

void HAL_Tone_Stop(uint8_t pin)
{
}

uint32_t HAL_Tone_Get_Frequency(uint8_t pin)
{
    return 0;
}

bool HAL_Tone_Is_Stopped(uint8_t pin)
{
    return true;
}
