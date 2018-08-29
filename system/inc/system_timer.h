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

#ifndef SYSTEM_TIMER_H__
#define SYSTEM_TIMER_H__

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*timer_callback_fn_t)(void);

/*!
 * \brief System Timer object description
 */
typedef struct SystemTimerEvent_s {
    uint32_t Timestamp;               //! Current timer value  [ms]
    uint32_t ReloadValue;             //! Timer delay value [ms]
    bool IsOneshot;                   //! Is the timer oneshot
    bool IsRunning;                   //! Is the timer currently running
    timer_callback_fn_t Callback;     //! Timer IRQ callback function
    struct SystemTimerEvent_s *Next;  //! Pointer to the next Timer object.
} SystemTimerEvent_t;

/*!
 * \brief Timer time variable definition
 */
#ifndef SystemTimerTime_t
typedef uint32_t SystemTimerTime_t;
#endif

/*!
 * \brief Set Timer Irq CallBack
 *
 * \remark Irq Callback must be called before starting the timer.
 *         this function set the hal timer irq callback.
 *
 */
void system_timer_set_irq_callback(void);

/*!
 * \brief Initializes the timer object
 *
 * \remark TimerSetValue function must be called before starting the timer.
 *         this function initializes timestamp and reload value at 0.
 *
 * \param [IN] TimerObject  Structure containing the timer object parameters
 * \param [IN] period       period of timer
 * \param [IN] callback     Function callback called at the end of the timeout
 * \param [IN] isOneshot    Oneshot or repeat
 */
void system_timer_init( SystemTimerEvent_t *TimerObject, SystemTimerTime_t period, timer_callback_fn_t callback , bool isOneshot);

/*!
 * \brief Starts and adds the timer object to the list of timer events
 *
 * \param [IN] TimerObject Structure containing the timer object parameters
 */
void system_timer_start( SystemTimerEvent_t *TimerObject );

/*!
 * \brief Stops and removes the timer object from the list of timer events
 *
 * \param [IN] TimerObject Structure containing the timer object parameters
 */
void system_timer_stop( SystemTimerEvent_t *TimerObject );

/*!
 * \brief Resets the timer object
 *
 * \param [IN] TimerObject Structure containing the timer object parameters
 */
void system_timer_reset( SystemTimerEvent_t *TimerObject );

/*!
 * \brief Set timer new timeout value
 *
 * \param [IN] TimerObject   Structure containing the timer object parameters
 * \param [IN] value New timer timeout value
 */
void system_timer_set_period( SystemTimerEvent_t *TimerObject, SystemTimerTime_t value );

/*!
 * \brief Return the Time elapsed of TimerObject
 *
 * \param [IN] TimerObject   Structure containing the timer object parameters
 * \param [IN] callback      Function callback called at the end of the timeout
 */
void system_timer_attach_interrupt( SystemTimerEvent_t *TimerObject, timer_callback_fn_t callback_fn );

/*!
 * \brief Return the active status of TimerObject
 *
 * \param [IN] TimerObject   Structure containing the timer object parameters
 * \retval time              returns the active status of time
 */
bool system_timer_is_active( SystemTimerEvent_t *TimerObject );

/*!
 * \brief Return the Time elapsed of TimerObject
 *
 * \param [IN] TimerObject   Structure containing the timer object parameters
 * \retval time              returns elapsed time
 */
SystemTimerTime_t system_timer_get_elapsed_time( SystemTimerEvent_t *TimerObject );

/*!
 * \brief Return the Time ramaining of TimerObject
 *
 * \param [IN] TimerObject   Structure containing the timer object parameters
 * \retval time              returns remaining time
 */
SystemTimerTime_t system_timer_get_remaining_time( SystemTimerEvent_t *TimerObject );

#ifdef __cplusplus
}
#endif

#endif // SYSTEM_TIMER_H__

