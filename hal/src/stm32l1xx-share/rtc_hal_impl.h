/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech

Description: MCU RTC timer and low power modes management

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis and Gregory Cristian
*/
#ifndef __RTC_HAL_IMPL_H__
#define __RTC_HAL_IMPL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "hw_config.h"

/*!
 * RTC timer context
 */
typedef struct RtcCalendar_s
{
    uint16_t CalendarCentury;     //! Keep track of century value
    RTC_DateTypeDef CalendarDate; //! Reference time in calendar format
    RTC_TimeTypeDef CalendarTime; //! Reference date in calendar format
} RtcCalendar_t;

RtcCalendar_t RtcGetCalendar( void );
RtcCalendar_t RtcComputeTimerTimeToAlarmTick( time_t timeCounter, RtcCalendar_t now );
RtcCalendar_t RtcConvertTimerTimeToCalendarTick( time_t timeCounter );
time_t RtcConvertCalendarTickToTimerTime( RtcCalendar_t *calendar );

extern RTC_HandleTypeDef RtcHandle;

#ifdef __cplusplus
}
#endif

#endif // __RTC_BOARD_H__
