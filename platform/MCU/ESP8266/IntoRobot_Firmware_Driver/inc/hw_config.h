/**
 ******************************************************************************
 * @file    hw_config.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    22-Oct-2014
 * @brief   Hardware Configuration & Setup
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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


/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include "sdkqueue.h"
#include "hw_ticks.h"
#include "esp8266_timer.h"
#include "esp8266_peri.h"
#include "uart.h"
#include "stdlib_noniso.h"
#include "service_debug.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "c_types.h"
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"
#include "cont.h"
#include "espconn.h"

#ifdef __cplusplus
}
#endif

typedef enum {DISABLE = 0, ENABLE = !DISABLE} FunctionalState;

#define IS_FUNCTIONAL_STATE(STATE) (((STATE) == DISABLE) || ((STATE) == ENABLE))
#define __IO    volatile             /*!< Defines 'read / write' permissions              */

#endif  /*HW_CONFIG_H_*/