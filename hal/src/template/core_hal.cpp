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

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include "core_hal.h"
#include "watchdog_hal.h"
#include "rng_hal.h"
#include "rgbled_hal.h"
#include "ota_flash_hal.h"
#include "gpio_hal.h"
#include "interrupts_hal.h"
#include "hw_config.h"
#include "rtc_hal.h"
#include "stm32l1xx_it.h"

/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Extern variables ----------------------------------------------------------*/

int main()
{
    while(1);
}

void HAL_Core_Init(void)
{
}

void HAL_Core_Config(void)
{
}

void HAL_Core_Setup(void)
{
}

void HAL_Core_System_Reset(void)
{
}

void HAL_Core_System_Reset_Ex(int reason, uint32_t data, void *reserved)
{
}

void HAL_Core_Factory_Reset(void)
{
}

void HAL_Core_Enter_Safe_Mode(void* reserved)
{
}

void HAL_Core_Enter_Bootloader(bool persist)
{
}

void HAL_Core_Enter_Stop_Mode(uint16_t wakeUpPin, uint16_t edgeTriggerMode, long seconds)
{
}

void HAL_Core_Execute_Stop_Mode(void)
{
}

void HAL_Core_Enter_Standby_Mode(void)
{
}

void HAL_Core_Execute_Standby_Mode(void)
{
}

void HAL_Core_Enter_Config(void)
{
}

void HAL_Core_Exit_Config(void)
{
}
