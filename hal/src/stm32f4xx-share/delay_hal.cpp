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

#include "delay_hal.h"
#include "hw_config.h"
#include "watchdog_hal.h"
#include "concurrent_hal.h"


/**
  * @brief  Delay milliseconds
  *         This should consider OS used case.
  * @param  nTime: specifies the delay time length, in milliseconds.
  * @retval None
  */
void HAL_Delay_Milliseconds(uint32_t millis)
{
#if 1
    os_delay(millis);
#else
    system_tick_t start_millis = HAL_Timer_Get_Milli_Seconds();

    while (1)
    {
        HAL_IWDG_Feed();

        system_tick_t elapsed_millis = HAL_Timer_Get_Milli_Seconds() - start_millis;

        if (elapsed_millis > millis)
        {
            break;
        }
    }
#endif
}

/**
  * @brief  delay time in microseconds using 32-bit DWT->CYCCNT
  * @param  uSec: specifies the delay time length, in milliseconds.
  * @retval None
  */
void HAL_Delay_Microseconds(uint32_t micros)
{
    volatile uint32_t DWT_START = DWT->CYCCNT;
    // keep DWT_TOTAL from overflowing (max 59.652323s w/72MHz SystemCoreClock)
    if (micros > (UINT_MAX / SYSTEM_US_TICKS))
    {
        micros = (UINT_MAX / SYSTEM_US_TICKS);
    }

    volatile uint32_t DWT_TOTAL = (SYSTEM_US_TICKS * micros);

    while((DWT->CYCCNT - DWT_START) < DWT_TOTAL)
    {
        HAL_IWDG_Feed();
    }
}
