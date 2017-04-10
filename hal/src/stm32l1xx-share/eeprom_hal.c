/**
 ******************************************************************************
 * @file    eeprom_hal.c
 * @author  Matthew McGowan
 * @version V1.0.0
 * @date    27-Sept-2014
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
#include "eeprom_hal.h"
#include "hw_config.h"


//L151 EEPROM固定起始地址
#define EEPROM_START_ADDRESS    ((uint32_t)0x08080000)

/* EEPROM Emulation Size */
#define EEPROM_SIZE             ((uint8_t)0x64)       /* 100 bytes (Max 255/0xFF bytes) */


static uint8_t EEPROM_ReadByte(uint32_t address);
static void EEPROM_WriteByte(uint32_t address, uint8_t dataVal);


static uint8_t EEPROM_ReadByte(uint32_t address)
{
    uint32_t rAddress = EEPROM_START_ADDRESS+address;
    uint8_t  tmp = 0;
    if(IS_FLASH_DATA_ADDRESS(rAddress))
    {
        HAL_FLASHEx_DATAEEPROM_Unlock();
        while(__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY));
        tmp = *(__IO uint8_t*)rAddress;
        HAL_FLASHEx_DATAEEPROM_Lock();
    }
    return tmp;
}

static void EEPROM_WriteByte(uint32_t address, uint8_t dataVal)
{
    uint32_t wAddress = EEPROM_START_ADDRESS+address;
    if(IS_FLASH_DATA_ADDRESS(wAddress))
    {
        HAL_FLASHEx_DATAEEPROM_Unlock();
        while(HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEERASEDATA_BYTE,wAddress,dataVal) != HAL_OK);
        HAL_FLASHEx_DATAEEPROM_Lock();
    }
}


void HAL_EEPROM_Init(void)
{
   /* EEPROM_Init();
    for(uint16_t i = 0; i < EEPROM_SIZE; i++)
    {
        EepromAddressTab[i] = i;
    }
    DEBUG("eeprom init");
    */
}

uint8_t HAL_EEPROM_Read(uint32_t address)
{
    if (address < EEPROM_SIZE)
    {
       return EEPROM_ReadByte(address);
    }
    else
    {
        return 0xFF;
    }
}

void HAL_EEPROM_Write(uint32_t address, uint8_t data)
{
    if (address < EEPROM_SIZE)
    {
        EEPROM_WriteByte(address,data);
    }
}

size_t HAL_EEPROM_Length()
{
   return EEPROM_SIZE;
}
