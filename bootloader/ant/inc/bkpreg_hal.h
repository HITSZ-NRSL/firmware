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
#ifndef __BKPPREG_HAL_H
#define __BKPPREG_HAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Exported types ------------------------------------------------------------*/
typedef enum
{
    BKP_DR_01 = 0x01,
    BKP_DR_02 = 0x02,
    BKP_DR_03 = 0x03,
    BKP_DR_04 = 0x04,
    BKP_DR_05 = 0x05,
    BKP_DR_06 = 0x06,
    BKP_DR_07 = 0x07,
    BKP_DR_08 = 0x08,
    BKP_DR_09 = 0x09,
    BKP_DR_10 = 0x10
} BKP_DR_TypeDef;

#ifdef __cplusplus
extern "C" {
#endif

int32_t HAL_Core_Backup_Register(uint32_t BKP_DR);
void HAL_Core_Write_Backup_Register(uint32_t BKP_DR, uint32_t Data);
uint32_t HAL_Core_Read_Backup_Register(uint32_t BKP_DR);

int HAL_System_Backup_Save(size_t offset, const void* buffer, size_t length, void* reserved);
int HAL_System_Backup_Restore(size_t offset, void* buffer, size_t max_length, size_t* length, void* reserved);

#ifdef __cplusplus
}
#endif

#endif
