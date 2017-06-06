/*
  pwm.c - analogWrite implementation for esp8266

  Copyright (c) 2015 Hristo Gochkov. All rights reserved.
  This file is part of the esp8266 core for Arduino environment.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "esp8266-hal-servotimers.h"
#include "esp8266-hal-servo.h"
#include "service_debug.h"


#if !defined (SERVO_EXCLUDE_TIMER0)

volatile uint32_t _cycleStartTimer0;
volatile int8_t _currentChannelTimer0;


void ESP8266ServoTimer0(void)
{
    ESP8266setEndOfCycleTimer0();
}

uint32_t ESP8266usToTicksTimer0(uint32_t us)
{
    return (clockCyclesPerMicrosecond() * us);     // converts microseconds to tick
}
uint32_t ESP8266ticksToUsTimer0(uint32_t ticks)
{
    return (ticks / clockCyclesPerMicrosecond()); // converts from ticks back to microseconds
}

void ESP8266InitInterruptTimer0(timercallback handler)
{
    timer0_isr_init();
    timer0_attachInterrupt(handler);
}

// timer0 doesn't have a clear interrupt
void ESP8266ResetInterruptTimer0()
{

}

void ESP8266StopInterruptTimer0()
{
    timer0_detachInterrupt();
}

void ESP8266SetPulseCompareTimer0(uint32_t value)
{
    timer0_write(getCycleCount() + value);
}

void ESP8266SetCycleCompareTimer0(uint32_t value)
{
    timer0_write(_cycleStartTimer0 + value);
}

uint32_t ESP8266GetCycleCountTimer0()
{
    return getCycleCount() - _cycleStartTimer0;
}


void ESP8266StartCycleTimer0()
{
    _cycleStartTimer0 = getCycleCount();
    _currentChannelTimer0 = 0;
}

int8_t ESP8266getCurrentChannelTimer0()
{
    return _currentChannelTimer0;
}

void ESP8266nextChannelTimer0()
{
    _currentChannelTimer0++;
}

void ESP8266setEndOfCycleTimer0()
{
    _currentChannelTimer0 = -1;
}

bool ESP8266isEndOfCycleTimer0()
{
    return (_currentChannelTimer0 == -1);
}

ServoTimerSequence ESP8266timerIdTimer0()
{
    return ServoTimerSequence_Timer0;
}

servo_timer_t  s_servoTimer0 = {
    ESP8266ServoTimer0,
    ESP8266usToTicksTimer0,
    ESP8266ticksToUsTimer0,
    ESP8266InitInterruptTimer0,
    ESP8266ResetInterruptTimer0,
    ESP8266StopInterruptTimer0,
    ESP8266SetPulseCompareTimer0,
    ESP8266SetCycleCompareTimer0,
    ESP8266GetCycleCountTimer0,
    ESP8266StartCycleTimer0,
    ESP8266getCurrentChannelTimer0,
    ESP8266nextChannelTimer0,
    ESP8266setEndOfCycleTimer0,
    ESP8266isEndOfCycleTimer0,
    ESP8266timerIdTimer0
};
#endif


#if !defined (SERVO_EXCLUDE_TIMER1)

#define TIMER1_TICKS_PER_US (APB_CLK_FREQ / 1000000L)

volatile uint32_t _cycleTicksTimer1;
volatile int8_t _currentChannelTimer1;

void ESP8266ServoTimer1()
{
    ESP8266setEndOfCycleTimer1();
}

uint32_t ESP8266usToTicksTimer1(uint32_t us)
{
    return (TIMER1_TICKS_PER_US / 16 * us);     // converts microseconds to tick
}

uint32_t ESP8266ticksToUsTimer1(uint32_t ticks)
{
    return (ticks / TIMER1_TICKS_PER_US * 16); // converts from ticks back to microseconds
}

void ESP8266InitInterruptTimer1(timercallback handler)
{
    timer1_isr_init();
    timer1_attachInterrupt(handler);
    timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
    timer1_write(ESP8266usToTicksTimer1(REFRESH_INTERVAL));
}

void ESP8266ResetInterruptTimer1()
{

}

void ESP8266StopInterruptTimer1()
{
    timer1_detachInterrupt();
}

void ESP8266SetPulseCompareTimer1(uint32_t value)
{
    _cycleTicksTimer1 += value;
    timer1_write(value);
}

void ESP8266SetCycleCompareTimer1(uint32_t value)
{
    if (value <= _cycleTicksTimer1)
    {
        value = 1;
    }
    else
    {
        value -= _cycleTicksTimer1;
    }
    timer1_write(value);
}

uint32_t ESP8266GetCycleCountTimer1()
{
    return _cycleTicksTimer1;
}


void ESP8266StartCycleTimer1()
{
    _cycleTicksTimer1 = 0;
    _currentChannelTimer1 = 0;
}

int8_t ESP8266getCurrentChannelTimer1()
{
    return _currentChannelTimer1;
}

void ESP8266nextChannelTimer1()
{
    _currentChannelTimer1++;
}

void ESP8266setEndOfCycleTimer1()
{
    _currentChannelTimer1 = -1;
}

bool ESP8266isEndOfCycleTimer1()
{
    return (_currentChannelTimer1 == -1);
}

ServoTimerSequence ESP8266timerIdTimer1()
{
    return ServoTimerSequence_Timer1;
}


servo_timer_t  s_servoTimer1 = {
    ESP8266ServoTimer1,
    ESP8266usToTicksTimer1,
    ESP8266ticksToUsTimer1,
    ESP8266InitInterruptTimer1,
    ESP8266ResetInterruptTimer1,
    ESP8266StopInterruptTimer1,
    ESP8266SetPulseCompareTimer1,
    ESP8266SetCycleCompareTimer1,
    ESP8266GetCycleCountTimer1,
    ESP8266StartCycleTimer1,
    ESP8266getCurrentChannelTimer1,
    ESP8266nextChannelTimer1,
    ESP8266setEndOfCycleTimer1,
    ESP8266isEndOfCycleTimer1,
    ESP8266timerIdTimer1
};

#endif
