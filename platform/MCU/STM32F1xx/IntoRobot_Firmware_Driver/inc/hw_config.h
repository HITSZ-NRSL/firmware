/**
 ******************************************************************************
 * @file    hw_config.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    22-Oct-2014
 * @brief   Hardware Configuration & Setup
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
#ifndef HW_CONFIG_H_
#define HW_CONFIG_H_


#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <math.h>
#include "platform_config.h"
#include "sdkqueue.h"
#include "usbd_cdc_desc.h"
#include "usbd_cdc_if.h"
#include "usbd_dfu_desc.h"
#include "usbd_dfu_if.h"
#include "hw_ticks.h"
#include "flash_mal.h"
#include "system_tick_hal.h"
#include "stdlib_noniso.h"
#if defined(HAS_SERIAL_FLASH)
#include "spi_flash.h"
#endif
#include "molmc_log.h"


/* Exported types ------------------------------------------------------------*/
#define NVIC_VectTab_RAM             SRAM_BASE
#define NVIC_VectTab_FLASH           FLASH_BASE


/* Exported constants --------------------------------------------------------*/

/* Exported macros ------------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
void Set_System(void);
void NVIC_SetVectorTable(uint32_t NVIC_VectTab, uint32_t Offset);

void USB_Cable_Initial(void);
void USB_Cable_Config(FunctionalState NewState);

/* External variables --------------------------------------------------------*/



#ifdef __cplusplus
}
#endif

#endif  /*HW_CONFIG_H_*/
