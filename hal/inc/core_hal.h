/**
 ******************************************************************************
 * @file    core_hal.h
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CORE_HAL_H
#define __CORE_HAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
//Following is normally defined via "CFLAGS += -DDFU_BUILD_ENABLE" in makefile
#ifndef DFU_BUILD_ENABLE
#define DFU_BUILD_ENABLE
#endif

/*
 * Use the JTAG IOs as standard GPIOs (D3 to D7)
 * Note that once the JTAG IOs are disabled, the connection with the host debugger
 * is lost and cannot be re-established as long as the JTAG IOs remain disabled.
 */
#ifdef USE_SWD_JTAG
#define SWD_JTAG_ENABLE
#else
#ifdef USE_SWD
#define SWD_ENABLE_JTAG_DISABLE
#else
#define SWD_JTAG_DISABLE
#endif
#endif

/* Exported functions --------------------------------------------------------*/
#include "watchdog_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct runtime_info_t {
    uint16_t size;              /* Size of this struct. */
    uint16_t flags;             /* reserved, set to 0. */
    uint32_t freeheap;          /* Amount of guaranteed heap memory available. */
    uint32_t system_version;
} runtime_info_t;

void HAL_Core_Init(void);
void HAL_Core_Config(void);
void HAL_Core_System_Reset(void);
void HAL_Core_Enter_DFU_Mode(bool persist);
void HAL_Core_Enter_Com_Mode(void);
void HAL_Core_Enter_Factory_Reset_Mode(void);
void HAL_Core_Enter_Ota_Update_Mode(void);
void HAL_Core_Enter_Bootloader(bool persist);
void HAL_Core_Enter_Stop_Mode(uint16_t wakeUpPin, uint16_t edgeTriggerMode, long seconds);
void HAL_Core_Execute_Stop_Mode(void);
void HAL_Core_Enter_Standby_Mode(uint32_t seconds, void* reserved);
void HAL_Core_Execute_Standby_Mode(void);
void HAL_Core_System_Yield(void);
uint32_t HAL_Core_Runtime_Info(runtime_info_t* info, void* reserved);
void HAL_Core_Enter_Config(void);
void HAL_Core_Exit_Config(void);

// 非hal层函数声明
extern void app_setup_and_loop(void);
extern void app_setup_and_loop_initial(bool *threaded);
extern void app_loop(bool threaded);
extern void HAL_SysTick_Handler(void);
//Externally defined function that is called before user constructors.
extern void module_user_init_hook(void);

#ifdef __cplusplus
}
#endif

#endif  /* __CORE_HAL_H */
