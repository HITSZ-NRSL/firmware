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

#include "platforms.h"
/* Includes ------------------------------------------------------------------*/
#include "hw_config.h"
#include "rtc_hal.h"

RTC_HandleTypeDef RtcHandle;

/**
  * @brief RTC MSP Initialization
  *        This function configures the hardware resources
  * @param hrtc: RTC handle pointer
  *
  * @note  Care must be taken when HAL_RCCEx_PeriphCLKConfig() is used to select
  *        the RTC clock source; in this case the Backup domain will be reset in
  *        order to modify the RTC Clock source, as consequence RTC registers (including
  *        the backup registers) and RCC_BDCR register are set to their reset values.
  *
  * @retval None
  */
void HAL_RTC_MspInit(RTC_HandleTypeDef *hrtc)
{
    RCC_OscInitTypeDef        RCC_OscInitStruct;
    RCC_PeriphCLKInitTypeDef  PeriphClkInitStruct;

    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();

    /* Enable BKP CLK enable for backup registers */
    /* __HAL_RCC_BKP_CLK_ENABLE(); */

    /*##-1- Configure LSE as RTC clock source ###################################*/
    RCC_OscInitStruct.OscillatorType =  RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_LSE;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    RCC_OscInitStruct.LSEState = RCC_LSE_ON;
    RCC_OscInitStruct.LSIState = RCC_LSI_OFF;
    if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        DEBUG("RCC_OscConfg Error\r\n");
    }

    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
    if(HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
        DEBUG("RCCEx_PeriphCLKConfig Error\r\n");
    }
    /*##-2- Enable RTC peripheral Clocks #######################################*/
    /* Enable RTC Clock */
    __HAL_RCC_RTC_ENABLE();
    /*##-3- Configure the NVIC for RTC Alarm ###################################*/
    HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 0x0F, 0);
    HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);
    DEBUG("RCCEx_PeriphCLKConfig ok\r\n");
}
/**
 * @brief RTC MSP De-Initialization
 *        This function frees the hardware resources
 *          - Disable the Peripheral's clock
 * @param hrtc: RTC handle pointer
 * @retval None
 */
void HAL_RTC_MspDeInit(RTC_HandleTypeDef *hrtc)
{
    /*##-1- Reset peripherals ##################################################*/
    __HAL_RCC_RTC_DISABLE();

    /*##-2- Disables the PWR Clock and Disables access to the backup domain ###################################*/
    HAL_PWR_DisableBkUpAccess();
    __HAL_RCC_PWR_CLK_DISABLE();
}

static void RTC_CalendarAlarmConfig(void)
{
    RTC_DateTypeDef  sdatestructure;
    RTC_TimeTypeDef  stimestructure;
    RTC_AlarmTypeDef salarmstructure;

    /*##-1- Configure the Date #################################################*/
    /* Set Date: Tuesday February 18th 2014 */
    sdatestructure.Year = 0x14;
    sdatestructure.Month = RTC_MONTH_FEBRUARY;
    sdatestructure.Date = 0x18;
    sdatestructure.WeekDay = RTC_WEEKDAY_TUESDAY;

    if(HAL_RTC_SetDate(&RtcHandle,&sdatestructure,RTC_FORMAT_BCD) != HAL_OK)
    {
        /* Initialization Error */
        /* Error_Handler();  */
    }

    /*##-2- Configure the Time #################################################*/
    /* Set Time: 02:20:00 */
    stimestructure.Hours = 0x02;
    stimestructure.Minutes = 0x20;
    stimestructure.Seconds = 0x00;

    if(HAL_RTC_SetTime(&RtcHandle,&stimestructure,RTC_FORMAT_BCD) != HAL_OK)
    {
        /* Initialization Error */
        /* Error_Handler();  */
    }
}

void HAL_RTC_Initial(void)
{
    /*##-1- Configure the RTC peripheral #######################################*/
    /* Configure RTC prescaler and RTC data registers */
    RtcHandle.Instance          = RTC;
    RtcHandle.Init.AsynchPrediv = RTC_AUTO_1_SECOND;

    if (HAL_RTC_Init(&RtcHandle) != HAL_OK)
    {
        DEBUG("RTC Init Error!\r\n");
    }
    RTC_CalendarAlarmConfig();
    /* DEBUG("rtc inintilaze\r\n"); */
}

time_t HAL_RTC_Get_UnixTime(void)
{
    RTC_DateTypeDef sdatestructureget;
    RTC_TimeTypeDef stimestructureget;
    struct tm tmstruct;
    memset(&tmstruct, 0, sizeof(struct tm));

    /* Get the RTC current Time */
    HAL_RTC_GetTime(&RtcHandle, &stimestructureget, RTC_FORMAT_BIN);
    /* Get the RTC current Date */
    HAL_RTC_GetDate(&RtcHandle, &sdatestructureget, RTC_FORMAT_BIN);
    tmstruct.tm_year = 2000 + sdatestructureget.Year - 1900;
    tmstruct.tm_mon  = sdatestructureget.Month - 1;
    tmstruct.tm_mday = sdatestructureget.Date;
    tmstruct.tm_hour = stimestructureget.Hours;
    tmstruct.tm_min  = stimestructureget.Minutes;
    tmstruct.tm_sec  = stimestructureget.Seconds;
    time_t t = mktime( &tmstruct );
    return t;
}

static int _pow(int base, int exponent)
{
    int result = 1;
    int i = 0;

    for (i = 0; i < exponent; i++) {
        result *= base;
    }
    return result;
}

/*
 * @brief Direct transform dec data to hex data, for the Set_UnixTime function.
 *        This function not convert the data. Example: dec:16 -> hex:0x16
 * @param decData: The input decimal data
 * @retral The output hexadecimal data
 */
static int _dec2hex(uint8_t decData)
{
    int hexData  = 0;
    uint8_t iCount   = 0;
    while( (decData / 10) || (decData % 10) ) {
        hexData = hexData +  (decData % 10) * _pow(16, iCount);
        if (decData < 10) {
            break;
        }
        decData = decData / 10;
        iCount++;
    }
    return hexData;
}

void HAL_RTC_Set_UnixTime(time_t value)
{
    struct tm *tmTemp = gmtime( &value );
    RTC_DateTypeDef sdatestructure;
    RTC_TimeTypeDef stimestructure;

    /*##-1- Configure the Date #################################################*/
    /* Set Date: Friday January 1st 2016 */
    sdatestructure.Year    = _dec2hex(tmTemp->tm_year + 1900 -2000);
    sdatestructure.Month   = _dec2hex(tmTemp->tm_mon + 1);
    sdatestructure.Date    = _dec2hex(tmTemp->tm_mday);
    sdatestructure.WeekDay = RTC_WEEKDAY_FRIDAY;
    if(HAL_RTC_SetDate(&RtcHandle,&sdatestructure,RTC_FORMAT_BCD) != HAL_OK)
    {
        DEBUG("RTC Set_UnixTime SetDate failed!\r\n");
    }

    /*##-2- Configure the Time #################################################*/
    /* Set Time: 00:00:00 */
    stimestructure.Hours   = _dec2hex(tmTemp->tm_hour);
    stimestructure.Minutes = _dec2hex(tmTemp->tm_min);
    stimestructure.Seconds = _dec2hex(tmTemp->tm_sec);
    if (HAL_RTC_SetTime(&RtcHandle, &stimestructure, RTC_FORMAT_BCD) != HAL_OK)
    {
        DEBUG("RTC Set_UnixTime SetTime failed!\r\n");
    }
}

void HAL_RTC_Set_UnixAlarm(time_t value)
{
    HAL_NVIC_DisableIRQ(RTC_Alarm_IRQn);
    RTC_AlarmTypeDef salarmstructure;

    time_t alarm_time = HAL_RTC_Get_UnixTime() + value;
    struct tm *tmTemp = gmtime( &value );

    /*##-- Configure the RTC Alarm peripheral #################################*/
    /*  /\* Set Alam to 00:00:20 */
    salarmstructure.Alarm = RTC_ALARM_A;
    salarmstructure.AlarmTime.Hours = 0x02;
    salarmstructure.AlarmTime.Minutes = 0x20;
    salarmstructure.AlarmTime.Seconds = 0x30;

    if(HAL_RTC_SetAlarm_IT(&RtcHandle,&salarmstructure,RTC_FORMAT_BCD) != HAL_OK)
    {
        /* Initialization Error */
        DEBUG("RTC CalendarAlarmConfig SetAlarm Error!\r\n");
    }
}

void HAL_RTC_Cancel_UnixAlarm(void)
{
    HAL_NVIC_DisableIRQ(RTC_Alarm_IRQn);
}

uint8_t HAL_RTC_Time_Is_Valid(void* reserved)
{
    return 0;
}

void HAL_RTC_SetCallbacks(const void (*handle)(), void* reserved)
{
}

void RTC_Alarm_IRQHandler(void)
{
    HAL_RTC_AlarmIRQHandler(&RtcHandle);
}

