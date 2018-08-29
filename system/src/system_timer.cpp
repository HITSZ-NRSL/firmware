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

#include "interrupts_hal.h"
#include "timer_hal.h"
#include "system_timer.h"
#include "molmc_log.h"

/*!
 * Timers list head pointer
 */
static SystemTimerEvent_t *SystemTimerListHead = NULL;

static uint8_t IrqNestLevel = 0;

/*!
 * \brief Sets a timeout with the duration "timestamp"
 *
 * \param [IN] timestamp Delay duration
 */
static void _system_timer_disable_irq( void )
{
    HAL_disable_irq( );
    IrqNestLevel++;
}

/*!
 * \brief Sets a timeout with the duration "timestamp"
 *
 * \param [IN] timestamp Delay duration
 */
static void _system_timer_enable_irq( void )
{
    IrqNestLevel--;
    if( IrqNestLevel == 0 ) {
        HAL_enable_irq(0);
    }
}

/*!
 * \brief Sets a timeout with the duration "timestamp"
 *
 * \param [IN] timestamp Delay duration
 */
static void _system_timer_set_timeout( SystemTimerEvent_t *TimerObject )
{
    HAL_Timer_Start(TimerObject->Timestamp);
}

/*!
 * \brief Read the timer value of the currently running timer
 *
 * \retval value current timer value
 */
static SystemTimerTime_t _system_timer_get_elapsed_time( void )
{
    return HAL_Timer_Get_ElapsedTime(  );
}

/*!
 * \brief Adds a timer to the list.
 *
 * \remark The list is automatically sorted. The list head always contains the
 *         next timer to expire.
 *
 * \param [IN]  TimerObject Timer object to be added to the list
 * \param [IN]  remainingTime Remaining time of the running head after which the object may be added
 */
static void _system_timer_insert_timer( SystemTimerEvent_t *TimerObject, uint32_t remainingTime )
{
    uint32_t aggregatedTimestamp = 0;      // hold the sum of timestamps
    uint32_t aggregatedTimestampNext = 0;  // hold the sum of timestamps up to the next event

    SystemTimerEvent_t* prev = SystemTimerListHead;
    SystemTimerEvent_t* cur = SystemTimerListHead->Next;

    if( cur == NULL ) { // TimerObject comes just after the head
        TimerObject->Timestamp -= remainingTime;
        prev->Next = TimerObject;
        TimerObject->Next = NULL;
    } else {
        aggregatedTimestamp = remainingTime;
        aggregatedTimestampNext = remainingTime + cur->Timestamp;

        while( prev != NULL ) {
            if( aggregatedTimestampNext > TimerObject->Timestamp ) {
                TimerObject->Timestamp -= aggregatedTimestamp;
                if( cur != NULL ) {
                    cur->Timestamp -= TimerObject->Timestamp;
                }
                prev->Next = TimerObject;
                TimerObject->Next = cur;
                break;
            } else {
                prev = cur;
                cur = cur->Next;
                if( cur == NULL ) { // TimerObject comes at the end of the list
                    aggregatedTimestamp = aggregatedTimestampNext;
                    TimerObject->Timestamp -= aggregatedTimestamp;
                    prev->Next = TimerObject;
                    TimerObject->Next = NULL;
                    break;
                } else {
                    aggregatedTimestamp = aggregatedTimestampNext;
                    aggregatedTimestampNext = aggregatedTimestampNext + cur->Timestamp;
                }
            }
        }
    }
}

/*!
 * \brief Adds or replace the head timer of the list.
 *
 * \remark The list is automatically sorted. The list head always contains the
 *         next timer to expire.
 *
 * \param [IN]  TimerObject Timer object to be become the new head
 * \param [IN]  remainingTime Remaining time of the previous head to be replaced
 */
static void _system_timer_insert_newhead_timer( SystemTimerEvent_t *TimerObject, uint32_t remainingTime )
{
    SystemTimerEvent_t* cur = SystemTimerListHead;

    if( cur != NULL ) {
        cur->Timestamp = remainingTime - TimerObject->Timestamp;
        cur->IsRunning = false;
    }

    TimerObject->Next = cur;
    TimerObject->IsRunning = true;
    SystemTimerListHead = TimerObject;
    _system_timer_set_timeout( SystemTimerListHead );
}

static void _system_timer_insert( SystemTimerEvent_t *TimerObject )
{
    uint32_t elapsedTime = 0;
    uint32_t remainingTime = 0;

    TimerObject->Timestamp = TimerObject->ReloadValue;
    TimerObject->IsRunning = false;

    if( SystemTimerListHead == NULL ) {
        _system_timer_insert_newhead_timer( TimerObject, TimerObject->Timestamp );
    } else {
        if( SystemTimerListHead->IsRunning == true ) {
            elapsedTime = _system_timer_get_elapsed_time( );
            if( elapsedTime > SystemTimerListHead->Timestamp ) {
                elapsedTime = SystemTimerListHead->Timestamp; // security but should never occur
            }
            remainingTime = SystemTimerListHead->Timestamp - elapsedTime;
        } else {
            remainingTime = SystemTimerListHead->Timestamp;
        }

        if( TimerObject->Timestamp < remainingTime ) {
            _system_timer_insert_newhead_timer( TimerObject, remainingTime );
        } else {
            _system_timer_insert_timer( TimerObject, remainingTime );
        }
    }
}

/*!
 * \brief Check if the Object to be added is not already in the list
 *
 * \param [IN] timestamp Delay duration
 * \retval true (the object is already in the list) or false
 */
static bool _system_timer_exists( SystemTimerEvent_t *TimerObject )
{
    SystemTimerEvent_t* cur = SystemTimerListHead;

    while( cur != NULL ) {
        if( cur == TimerObject ) {
            return true;
        }
        cur = cur->Next;
    }
    return false;
}

static void _system_timer_irq_handler( void )
{
    uint32_t elapsedTime = 0;

    // Early out when SystemTimerListHead is null to prevent null pointer
    if ( SystemTimerListHead == NULL ) {
        return;
    }

    elapsedTime = _system_timer_get_elapsed_time( );

    if( elapsedTime >= SystemTimerListHead->Timestamp ) {
        SystemTimerListHead->Timestamp = 0;
    } else {
        SystemTimerListHead->Timestamp -= elapsedTime;
    }

    SystemTimerListHead->IsRunning = false;

    while( ( SystemTimerListHead != NULL ) && ( SystemTimerListHead->Timestamp == 0 ) ) {
        SystemTimerEvent_t* elapsedTimer = SystemTimerListHead;
        SystemTimerListHead = SystemTimerListHead->Next;

        if(elapsedTimer->IsOneshot == false) {
            _system_timer_insert( elapsedTimer );
        }

        if( elapsedTimer->Callback != NULL ) {
            elapsedTimer->Callback( );
        }
    }

    // start the next SystemTimerListHead if it exists
    if( SystemTimerListHead != NULL ) {
        if( SystemTimerListHead->IsRunning != true ) {
            SystemTimerListHead->IsRunning = true;
            _system_timer_set_timeout( SystemTimerListHead );
        }
    }
}

void system_timer_set_irq_callback(void)
{
    HAL_Timer_Set_Callback(&_system_timer_irq_handler);
}

void system_timer_init( SystemTimerEvent_t *TimerObject, SystemTimerTime_t period, timer_callback_fn_t callback , bool isOneshot)
{
    TimerObject->Timestamp = period;
    TimerObject->ReloadValue = period;
    TimerObject->IsOneshot = isOneshot;
    TimerObject->IsRunning = false;
    TimerObject->Callback = callback;
    TimerObject->Next = NULL;
}

void system_timer_start( SystemTimerEvent_t *TimerObject )
{
    _system_timer_disable_irq( );

    if( ( TimerObject == NULL ) || ( _system_timer_exists( TimerObject ) == true ) ) {
        _system_timer_enable_irq( );
        return;
    }
    _system_timer_insert( TimerObject );
    _system_timer_enable_irq( );
}

void system_timer_stop( SystemTimerEvent_t *TimerObject )
{
    uint32_t elapsedTime = 0;
    uint32_t remainingTime = 0;

    SystemTimerEvent_t* prev = SystemTimerListHead;
    SystemTimerEvent_t* cur = SystemTimerListHead;

    _system_timer_disable_irq( );
    // List is empty or the TimerObject to stop does not exist
    if( ( SystemTimerListHead == NULL ) || ( TimerObject == NULL ) ) {
        _system_timer_enable_irq( );
        return;
    }

    if( SystemTimerListHead == TimerObject ) { // Stop the Head
        if( SystemTimerListHead->IsRunning == true ) { // The head is already running
            elapsedTime = _system_timer_get_elapsed_time( );
            if( elapsedTime > TimerObject->Timestamp ) {
                elapsedTime = TimerObject->Timestamp;
            }

            remainingTime = TimerObject->Timestamp - elapsedTime;

            SystemTimerListHead->IsRunning = false;
            if( SystemTimerListHead->Next != NULL ) {
                SystemTimerListHead = SystemTimerListHead->Next;
                SystemTimerListHead->Timestamp += remainingTime;
                SystemTimerListHead->IsRunning = true;
                _system_timer_set_timeout( SystemTimerListHead );
            } else {
                SystemTimerListHead = NULL;
            }
        } else { // Stop the head before it is started
            if( SystemTimerListHead->Next != NULL ) {
                remainingTime = TimerObject->Timestamp;
                SystemTimerListHead = SystemTimerListHead->Next;
                SystemTimerListHead->Timestamp += remainingTime;
            } else {
                SystemTimerListHead = NULL;
            }
        }
    } else {// Stop an object within the list
        remainingTime = TimerObject->Timestamp;

        while( cur != NULL ) {
            if( cur == TimerObject ) {
                if( cur->Next != NULL ) {
                    cur = cur->Next;
                    prev->Next = cur;
                    cur->Timestamp += remainingTime;
                } else {
                    cur = NULL;
                    prev->Next = cur;
                }
                break;
            } else {
                prev = cur;
                cur = cur->Next;
            }
        }
    }
    _system_timer_enable_irq( );
}

void system_timer_reset( SystemTimerEvent_t *TimerObject )
{
    system_timer_stop( TimerObject );
    system_timer_start( TimerObject );
}

void system_timer_set_period( SystemTimerEvent_t *TimerObject, SystemTimerTime_t value )
{
    system_timer_stop( TimerObject );
    TimerObject->Timestamp = value;
    TimerObject->ReloadValue = value;
}

void system_timer_attach_interrupt( SystemTimerEvent_t *TimerObject, timer_callback_fn_t callback_fn )
{
    return TimerObject->Callback = callback_fn;
}

bool system_timer_is_active( SystemTimerEvent_t *TimerObject )
{
    return _system_timer_exists( TimerObject );
}

SystemTimerTime_t system_timer_get_elapsed_time( SystemTimerEvent_t *TimerObject )
{
    _system_timer_disable_irq( );
    // List is empty or the TimerObject to stop does not exist
    if( ( SystemTimerListHead == NULL ) || ( TimerObject == NULL ) ) {
        _system_timer_enable_irq( );
        return 0;
    }
    _system_timer_enable_irq( );

    return TimerObject->ReloadValue - system_timer_get_remaining_time( TimerObject );
}

SystemTimerTime_t system_timer_get_remaining_time( SystemTimerEvent_t *TimerObject )
{
    uint32_t elapsedTime = 0;
    uint32_t remainingTime = 0;

    SystemTimerEvent_t* cur = SystemTimerListHead;

    _system_timer_disable_irq( );
    // List is empty or the TimerObject to stop does not exist
    if( ( SystemTimerListHead == NULL ) || ( TimerObject == NULL ) ) {
        _system_timer_enable_irq( );
        return 0;
    }

    if( TimerObject->IsRunning == true ) {
        elapsedTime = _system_timer_get_elapsed_time( );
        if( elapsedTime > TimerObject->Timestamp ) {
            elapsedTime = TimerObject->Timestamp;
        }
        remainingTime = TimerObject->Timestamp - elapsedTime;
    } else {
        while( cur != NULL ) {
            if( cur->IsRunning == true ) {
                elapsedTime = _system_timer_get_elapsed_time( );
                if( elapsedTime > cur->Timestamp ) {
                    elapsedTime = cur->Timestamp;
                }
                remainingTime += cur->Timestamp - elapsedTime;
            } else {
                remainingTime += cur->Timestamp;
            }

            if( cur == TimerObject ) {
                break;
            } else {
                cur = cur->Next;
            }
        }
    }
    _system_timer_enable_irq( );
    return remainingTime;
}

