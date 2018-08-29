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

#ifndef WIRING_TIMER_H_
#define WIRING_TIMER_H_

#include "system_timer.h"

class Timer
{
    public:
        Timer(unsigned period, timer_callback_fn_t callback_fn, bool one_shot = false) {
            system_timer_init( &TimerObject, period, callback_fn, one_shot);
        }

        virtual ~Timer(void) {
        }

        bool start(void) {
            system_timer_start( &TimerObject );
            return true;
        }

        bool stop(void) {
            system_timer_stop( &TimerObject );
            return true;
        }

        bool reset(void) {
            system_timer_reset( &TimerObject );
            return true;
        }

        bool changePeriod(uint32_t period) {
            system_timer_set_period( &TimerObject, period );
            return true;
        }

        bool attachInterrupt(timer_callback_fn_t callback_fn) {
            system_timer_attach_interrupt( &TimerObject, callback_fn );
            return true;
        }

        bool isActive(void) {
            return system_timer_is_active( &TimerObject );
        }

        uint32_t getelapsedTime(void) {
            return system_timer_get_elapsed_time( &TimerObject );
        }

        uint32_t getRemainingTime(void) {
            return system_timer_get_remaining_time( &TimerObject );
        }

    private:
        SystemTimerEvent_t TimerObject;
};

#endif

