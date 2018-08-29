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
#include "wiring.h"
#include "wiring_ticks.h"
#include "adc_hal.h"
#include "system_task.h"

void setADCSampleTime(uint8_t ADC_SampleTime)
{
    HAL_ADC_Set_Sample_Time(ADC_SampleTime);
}

long map(long value, long fromStart, long fromEnd, long toStart, long toEnd)
{
    return (value - fromStart) * (toEnd - toStart) / (fromEnd - fromStart) + toStart;
}

void delay(unsigned long ms)
{
    system_delay_ms(ms,false);
}

uint32_t timerGetId(void)
{
    return millis();
}

bool timerIsEnd(uint32_t timerID, uint32_t time)
{
    volatile system_tick_t current_millis = millis();
    volatile long elapsed_millis = current_millis - timerID;

    //Check for wrapping
    if (elapsed_millis < 0) {
        elapsed_millis =  0xFFFFFFFF -timerID + current_millis;
    }

    if (elapsed_millis >= (long)time) {
        return true;
    }
    return false;
}
