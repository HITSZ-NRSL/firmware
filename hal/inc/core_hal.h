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
typedef enum
{
    PIN_RESET = 0x01,
    SOFTWARE_RESET = 0x02,
    WATCHDOG_RESET = 0x03,
    POWER_MANAGEMENT_RESET = 0x04,
    POWER_DOWN_RESET = 0x05,
    POWER_BROWNOUT_RESET = 0x06
} RESET_TypeDef;

// Reason codes are exposed under identifier names via the cloud - ensure the mapping is
// updated for newly added reason codes
typedef enum System_Reset_Reason
{
    RESET_REASON_NONE = 0,
    RESET_REASON_UNKNOWN = 10, // Unspecified reason
    // Hardware
    RESET_REASON_PIN_RESET = 20, // Reset from the NRST pin
    RESET_REASON_POWER_MANAGEMENT = 30, // Low-power management reset
    RESET_REASON_POWER_DOWN = 40, // Power-down reset
    RESET_REASON_POWER_BROWNOUT = 50, // Brownout reset
    RESET_REASON_WATCHDOG = 60, // Watchdog reset
    // Software
    RESET_REASON_UPDATE = 70, // Successful firmware update
    RESET_REASON_UPDATE_ERROR = 80, // Generic update error
    RESET_REASON_UPDATE_TIMEOUT = 90, // Update timeout
    RESET_REASON_FACTORY_RESET = 100, // Factory reset requested
    RESET_REASON_SAFE_MODE = 110, // Safe mode requested
    RESET_REASON_DFU_MODE = 120, // DFU mode requested
    RESET_REASON_PANIC = 130, // System panic (additional data may contain panic code)
    RESET_REASON_USER = 140 // User-requested reset
} System_Reset_Reason;

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
/*
 * Use Independent Watchdog to force a system reset when a software error occurs
 * During JTAG program/debug, the Watchdog counting is disabled by debug configuration
 */
#define IWDG_RESET_ENABLE
#define TIMING_IWDG_RELOAD      1000 //1sec

/* Exported functions --------------------------------------------------------*/
#include "watchdog_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

int atexit(void (*func)()) __attribute__((weak));

void HAL_Core_Init(void);
void HAL_Core_Config(void);
void HAL_Core_Load_params(void);
bool HAL_Core_Validate_User_Module(void);
void HAL_Core_System_Reset(void);
void HAL_Core_Enter_DFU_Mode(bool persist);
void HAL_Core_Enter_Config_Mode(void);
void HAL_Core_Enter_Firmware_Recovery_Mode(void);
void HAL_Core_Enter_Com_Mode(void);
void HAL_Core_Enter_Factory_Reset_Mode(void);
void HAL_Core_Enter_Ota_Update_Mode(void);
void HAL_Core_Enter_Factory_All_Reset_Mode(void);


void HAL_Core_Enter_Safe_Mode(void* reserved);
void HAL_Core_Enter_Bootloader(bool persist);

typedef enum _BootloaderFlag_t {
    BOOTLOADER_FLAG_VERSION,
    BOOTLOADER_FLAG_STARTUP_MODE
} BootloaderFlag;

enum BootloaderFeaturesEnabled
{
    BL_FEATURE_SAFE_MODE = 1<<0,
    BL_FEATURE_DFU_MODE = 1<<1,
    BL_FEATURE_FACTORY_RESET = 1<<2,
    BL_BUTTON_FEATURES = (BL_FEATURE_SAFE_MODE|BL_FEATURE_DFU_MODE|BL_FEATURE_FACTORY_RESET)
};

uint16_t HAL_Bootloader_Get_Flag(BootloaderFlag flag);

//Following is currently defined in bootloader/src/core-vx/dfu_hal.c
//Move the definitions to core_hal.c and add hal as a dependency for bootloader
void HAL_Core_Set_System_Loop_Handler(void (*handler)(void));
void HAL_SysTick_Handler(void);

void HAL_Bootloader_Lock(bool lock);
uint16_t HAL_Core_Get_Subsys_Version(char* buffer, uint16_t len);
void HAL_Core_System_Loop(void);
void HAL_Core_System_Yield(void);
void HAL_Core_System_Loop_Control(bool state);

typedef struct runtime_info_t {
    uint16_t size;              /* Size of this struct. */
    uint16_t flags;             /* reserved, set to 0. */
    uint32_t freeheap;          /* Amount of guaranteed heap memory available. */
    uint32_t system_version;
} runtime_info_t;

uint32_t HAL_Core_Runtime_Info(runtime_info_t* info, void* reserved);

extern void app_setup_and_loop(void);
extern void app_setup_and_loop_initial(bool *threaded);
extern void app_loop(bool threaded);

typedef enum HAL_SystemClock
{
    SYSTEMCLOCK_PRIMARY,
    SYSTEMCLOCK_SPI
} HAL_SystemClock;

/**
 * Retrieves the
 * @param reserved
 * @return
 */
unsigned HAL_Core_System_Clock(HAL_SystemClock clock, void* reserved);

typedef enum HAL_Feature {
    FEATURE_RETAINED_MEMORY=1,       // [write only] retained memory on backup power
    FEATURE_WARM_START,              // [read only] set to true if previous retained memory contents are available]
	FEATURE_CLOUD_UDP,				// [read only] true if the UDP implementation should be used.
    FEATURE_RESET_INFO               // [read/write] enables handling of last reset info (may affect backup registers)
} HAL_Feature;

int HAL_Feature_Set(HAL_Feature feature, bool enabled);
bool HAL_Feature_Get(HAL_Feature feature);

/**
 * Externally defined function that is called before user constructors.
 */
extern void module_user_init_hook(void);

#ifdef __cplusplus
}
#endif

#endif  /* __CORE_HAL_H */
