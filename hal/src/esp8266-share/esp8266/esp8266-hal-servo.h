/*
  Servo.h - Interrupt driven Servo library for Esp8266 using timers
  Copyright (c) 2015 Michael C. Miller. All right reserved.

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


//   A servo is activated by creating an instance of the Servo class passing
//   the desired pin to the attach() method.
//   The servos are pulsed in the background using the value most recently
//   written using the write() method.
//
//   This library uses timer0 and timer1.
//   Note that timer0 may be repurposed when the first servo is attached.
//
//   Timers are seized as needed in groups of 12 servos - 24 servos use two
//   timers, there are only two timers for the esp8266 so the support stops here
//   The sequence used to sieze timers is defined in timers.h
//
//   The methods are:
//
//   Servo - Class for manipulating servo motors connected to Arduino pins.
//
//   attach(pin )  - Attaches a servo motor to an i/o pin.
//   attach(pin, min, max  ) - Attaches to a pin setting min and max values in microseconds
//   default min is 544, max is 2400
//
//   write()     - Sets the servo angle in degrees.  (invalid angle that is valid as pulse in microseconds is treated as microseconds)
//   writeMicroseconds() - Sets the servo pulse width in microseconds
//   read()      - Gets the last written servo pulse width as an angle between 0 and 180.
//   readMicroseconds()   - Gets the last written servo pulse width in microseconds. (was read_us() in first release)
//   attached()  - Returns true if there is a servo attached.
//   detach()    - Stops an attached servos from pulsing its i/o pin.

#ifndef _ESP8266_HAL_SERVO_H
#define _ESP8266_HAL_SERVO_H

#include "esp8266-hal-servotimers.h"

// the following are in us (microseconds)
#define MIN_PULSE_WIDTH       544     // the shortest pulse sent to a servo  
#define MAX_PULSE_WIDTH      2400     // the longest pulse sent to a servo 
#define DEFAULT_PULSE_WIDTH  1500     // default pulse width when servo is attached
#define REFRESH_INTERVAL    20000     // minumim time to refresh servos in microseconds 

// NOTE: to maintain a strict refresh interval the user needs to not exceede 8 servos 
#define SERVOS_PER_TIMER       12     // the maximum number of servos controlled by one timer 
#define MAX_SERVOS   (ServoTimerSequence_COUNT  * SERVOS_PER_TIMER)

#ifdef __cplusplus
extern "C" {
#endif

void ESP8266ServoInit(void);
uint8_t ESP8266ServoAttach(int pin);
void ESP8266ServoWrite(int8_t pin, int value);
void ESP8266ServoDetach(int8_t pin);
int ESP8266ServoRead(int8_t pin); // return the value as degrees

#ifdef __cplusplus
}
#endif

#endif
