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

#ifndef FLASH_MAP_H_
#define FLASH_MAP_H_

#define INTERNAL_FLASH_START            ((uint32_t)0x08000000)
#define INTERNAL_FLASH_SIZE             (0x20000)
#define INTERNAL_FLASH_END_ADDRESS      ((uint32_t)INTERNAL_FLASH_START+INTERNAL_FLASH_SIZE-1) //For 128KB Internal Flash

/*
 *  20k     bootloader
 *  1k      bootloader参数区
 *  3k      应用系统参数区
 *  104k    应用程序区
 */
#define BOOTLOADER_START_ADDR            ((uint32_t)0x08000000)
#define APPLICATION_START_ADDR           ((uint32_t)0x08005000)

#define EEPROM_START_ADDR                ((uint32_t)0x08008000)

#define BOOT_PARAMS_START_ADDR           ((uint32_t)0x08005000)
#define BOOT_PARAMS_END_ADDR             ((uint32_t)0x080053FF)
#define SYSTEM_PARAMS_START_ADDR         ((uint32_t)0x08005400)
#define SYSTEM_PARAMS_END_ADDR           ((uint32_t)0x08005FFF)

#endif /*FLASH_MAP_H_*/

