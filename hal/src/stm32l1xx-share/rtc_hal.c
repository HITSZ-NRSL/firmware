/**
 ******************************************************************************
 * @file    rtc_hal.c
 * @author  Satish Nair, Brett Walach
 * @version V1.0.0
 * @date    12-Sept-2014
 * @brief
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

/* Includes ------------------------------------------------------------------*/
#include "hw_config.h"
#include "rtc_hal.h"
#include "gpio_hal.h"
#include <math.h>
#include "rtc_hal_impl.h"
#include "interrupts_hal.h"
#include "ui_hal.h"

/* Private typedef -----------------------------------------------------------*/
typedef void(*HAL_RTC_Callbacks)(void);

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* static RTC_HandleTypeDef RtcHandle; */
static bool rtcState = true;
static bool RtcInitialized = false;
static HAL_RTC_Callbacks rtcCallbacks = NULL;

/* Extern variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/


// HAL_RTC_Init 回调函数
void HAL_RTC_MspInit(RTC_HandleTypeDef *hrtc)
{
    RCC_OscInitTypeDef        RCC_OscInitStruct;
    RCC_PeriphCLKInitTypeDef  PeriphClkInitStruct;

    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();

    RCC_OscInitStruct.OscillatorType =  RCC_OSCILLATORTYPE_LSE;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    RCC_OscInitStruct.LSEState = RCC_LSE_ON;
    if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        rtcState = false;
        return;
    }

    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
    if(HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
        rtcState = false;
        return;
    }
    __HAL_RCC_RTC_ENABLE();
}

// HAL_RTC_DeInit 回调函数
void HAL_RTC_MspDeInit(RTC_HandleTypeDef *hrtc)
{
    __HAL_RCC_RTC_DISABLE();
    HAL_PWR_DisableBkUpAccess();
    __HAL_RCC_PWR_CLK_DISABLE();
}

static void setRTCTime(RTC_TimeTypeDef* RTC_TimeStructure, RTC_DateTypeDef* RTC_DateStructure)
{
    if (HAL_RTC_SetTime(&RtcHandle, RTC_TimeStructure, RTC_FORMAT_BIN) != HAL_OK) {
        /* RTC Set Time failed */
    }

    if(HAL_RTC_SetDate(&RtcHandle, RTC_DateStructure,RTC_FORMAT_BIN) != HAL_OK) {
        /* RTC Set Date failed */
    }
}

/* Set date/time to 2000/01/01 00:00:00 */
static void HAL_RTC_Initialize_UnixTime(void)
{
    RTC_TimeTypeDef RTC_TimeStructure;
    RTC_DateTypeDef RTC_DateStructure;

    // Set Date: Friday 1st of January 2000
    RTC_DateStructure.Year = 0;
    RTC_DateStructure.Month = 1;
    RTC_DateStructure.Date = 1;
    RTC_DateStructure.WeekDay = RTC_WEEKDAY_SATURDAY;

    // Set Time: 00:00:00
    RTC_TimeStructure.Hours = 0;
    RTC_TimeStructure.Minutes = 0;
    RTC_TimeStructure.Seconds = 0;
    RTC_TimeStructure.TimeFormat = RTC_HOURFORMAT12_AM;
    RTC_TimeStructure.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    RTC_TimeStructure.StoreOperation = RTC_STOREOPERATION_RESET;

    setRTCTime(&RTC_TimeStructure, &RTC_DateStructure);
}

void HAL_RTC_Initial( void )
{
    if( RtcInitialized == true ) {
        return;
    }

    __HAL_RCC_RTC_ENABLE( );

    RtcHandle.Instance = RTC;
    RtcHandle.Init.HourFormat = RTC_HOURFORMAT_24;
    //设置32.768K的分频系数
    //Fclk = 32768/[(SynchPrediv+1)*(AsynchPrediv+1)]
    RtcHandle.Init.AsynchPrediv = 3;
    RtcHandle.Init.SynchPrediv = 3;
    RtcHandle.Init.OutPut = RTC_OUTPUT_DISABLE;
    RtcHandle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    RtcHandle.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
    if (HAL_RTC_Init(&RtcHandle) != HAL_OK) {
        /*RTC Init Error!*/
    }

    HAL_RTC_Initialize_UnixTime();

    // Enable Direct Read of the calendar registers (not through Shadow registers)
    HAL_RTCEx_EnableBypassShadow( &RtcHandle );

    HAL_NVIC_SetPriority( RTC_Alarm_IRQn, RTC_Alarm_IRQ_PRIORITY, 0 );
    HAL_NVIC_EnableIRQ( RTC_Alarm_IRQn );
    RtcInitialized = true;
}

//获取当前unix时间戳 单位：毫秒数
time_t HAL_RTC_Get_UnixTime(void)
{
    return RtcConvertCalendarTickToTimerTime(NULL);
}

//设置unix时间 单位： 毫秒
void HAL_RTC_Set_UnixTime(time_t value)
{
    RtcCalendar_t calendar = RtcConvertTimerTimeToCalendarTick(value);

    int32_t state = HAL_disable_irq();
    setRTCTime(&calendar.CalendarTime, &calendar.CalendarDate);
    HAL_enable_irq(state);
}

//设置一定时间后报警 单位： 毫秒
void HAL_RTC_Set_UnixAlarm(time_t value)
{
    RTC_AlarmTypeDef RTC_AlarmStructure;

    HAL_RTC_DeactivateAlarm( &RtcHandle, RTC_ALARM_A );
    HAL_RTCEx_DeactivateWakeUpTimer( &RtcHandle );

    RtcCalendar_t alarmTimer = RtcComputeTimerTimeToAlarmTick(value, RtcGetCalendar());

    RTC_AlarmStructure.Alarm = RTC_ALARM_A;
    RTC_AlarmStructure.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
    RTC_AlarmStructure.AlarmMask = RTC_ALARMMASK_NONE;
    RTC_AlarmStructure.AlarmTime.TimeFormat = RTC_HOURFORMAT12_AM;

    RTC_AlarmStructure.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
    RTC_AlarmStructure.AlarmTime.SubSeconds = 0;
    RTC_AlarmStructure.AlarmTime.Seconds = alarmTimer.CalendarTime.Seconds;
    RTC_AlarmStructure.AlarmTime.Minutes = alarmTimer.CalendarTime.Minutes;
    RTC_AlarmStructure.AlarmTime.Hours = alarmTimer.CalendarTime.Hours;
    RTC_AlarmStructure.AlarmDateWeekDay = alarmTimer.CalendarDate.Date;

    if( HAL_RTC_SetAlarm_IT( &RtcHandle, &RTC_AlarmStructure, RTC_FORMAT_BIN ) != HAL_OK ) {
        /* SetAlarm Error */
    }
}

void HAL_RTC_Cancel_UnixAlarm(void)
{
    HAL_NVIC_DisableIRQ(RTC_Alarm_IRQn);
}

uint8_t HAL_RTC_Time_Is_Valid(void* reserved)
{
    return rtcState;
}

void HAL_RTC_SetCallbacks(const void (*handle)(), void* reserved)
{
    rtcCallbacks = handle;
}

void RTC_Alarm_IRQHandler( void )
{
    HAL_RTC_AlarmIRQHandler( &RtcHandle );
    HAL_RTC_DeactivateAlarm( &RtcHandle, RTC_ALARM_A );
    if(rtcCallbacks) { //回调 TimerIrqHandler( )
        rtcCallbacks();
    }
}

