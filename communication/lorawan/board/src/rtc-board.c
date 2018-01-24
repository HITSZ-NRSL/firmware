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
#include <math.h>
#include "board.h"
#include "rtc-board.h"
#include "rtc_hal.h"
#include "rtc_hal_impl.h"

/*!
 * Save the calendar into ContextUnixTime to be able to calculate the elapsed time
 */
volatile TimerTime_t ContextUnixTime = 0; //记录alarm启动时的时间戳

void RtcSetTimeout( uint32_t timeout )
{
    ContextUnixTime = HAL_RTC_Get_UnixTime();
    HAL_RTC_Set_UnixAlarm(timeout);
}

TimerTime_t RtcGetTimerValue( void )
{
    return HAL_RTC_Get_UnixTime();
}

TimerTime_t RtcGetElapsedAlarmTime( void )
{
    TimerTime_t currentTime = HAL_RTC_Get_UnixTime();

    if( currentTime < ContextUnixTime )
    {
        return( currentTime + ( 0xFFFFFFFF - ContextUnixTime ) );
    }
    else
    {
        return( currentTime - ContextUnixTime );
    }
}

TimerTime_t RtcComputeFutureEventTime( TimerTime_t futureEventInTime )
{
    return( RtcGetTimerValue( ) + futureEventInTime );
}

TimerTime_t RtcComputeElapsedTime( TimerTime_t eventInTime )
{
    TimerTime_t elapsedTime = 0;

    // Needed at boot, cannot compute with 0 or elapsed time will be equal to current time
    if( eventInTime == 0 )
    {
        return 0;
    }

    elapsedTime = HAL_RTC_Get_UnixTime();

    if( elapsedTime < eventInTime)
    { // roll over of the counter
        return( elapsedTime + ( 0xFFFFFFFF - eventInTime) );
    }
    else
    {
        return( elapsedTime - eventInTime);
    }
}
