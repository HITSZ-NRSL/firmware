/*
  Copyright (c) 2015 Michael C. Miller. All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
  */

//
// Defines for timer abstractions used with  Servo library
//
// ServoTimerSequence enumerates the sequence that the timers should be allocated
// ServoTimerSequence_COUNT indicates how many timers are available.
//
#ifndef _ESP8266_HAL_SERVOTIMERS_H
#define _ESP8266_HAL_SERVOTIMERS_H

#include <stdint.h>
#include "esp8266_timer.h"
#include "eagle_soc.h"

//#define SERVO_EXCLUDE_TIMER0
//#define SERVO_EXCLUDE_TIMER1

#define getCycleCount() ((__extension__({static uint32_t ccount;__asm__ __volatile__("esync; rsr %0,ccount":"=a" (ccount));ccount;})))

typedef enum ServoTimerSequence_t {

#if !defined (SERVO_EXCLUDE_TIMER0)
    ServoTimerSequence_Timer0,
#endif

#if !defined (SERVO_EXCLUDE_TIMER1)
    ServoTimerSequence_Timer1,
#endif

    ServoTimerSequence_COUNT
}ServoTimerSequence;

typedef struct ESP8266ServoTimer
{
    void (*ServoTimer)(void);
    uint32_t (*usToTicks)(uint32_t us);

    uint32_t (*ticksToUs)(uint32_t ticks);
    void (*InitInterrupt)(timercallback handler);
    void (*ResetInterrupt)(); // timer0 1 doesn't have a clear interrupt

    void (*StopInterrupt)();
    void (*SetPulseCompare)(uint32_t value);
    void (*SetCycleCompare)(uint32_t value);
    uint32_t (*GetCycleCount)();
    void (*StartCycle)();
    int8_t (*getCurrentChannel)();
    void (*nextChannel)();
    void (*setEndOfCycle)();
    bool (*isEndOfCycle)();
    ServoTimerSequence (*timerId)();
}servo_timer_t;


#if !defined (SERVO_EXCLUDE_TIMER0)
extern servo_timer_t  s_servoTimer0;
#endif

#if !defined (SERVO_EXCLUDE_TIMER1)
extern servo_timer_t  s_servoTimer1;
#endif

#endif
