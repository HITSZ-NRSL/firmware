/**
 ******************************************************************************
 * @file    flash_mal.c
 * @author  Satish Nair, Matthew McGowan
 * @version V1.0.0
 * @date    30-Jan-2015
 * @brief   Media access layer for platform dependent flash interfaces
 ******************************************************************************
 Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation, either
 version 3 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this program; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "hw_config.h"
#include "flash_mal.h"
#include <string.h>

/* Private functions ---------------------------------------------------------*/

uint16_t FLASH_SectorToErase(flash_device_t device, uint32_t startAddress)
{
    uint16_t sector = 0xFFFF;
    sector = startAddress/SERIAL_FLASH_SECTOR_SIZE;
    return sector;
}

uint32_t FLASH_SectorsMask(uint32_t imageSize, uint32_t sectorSize)
{
    uint32_t numSectors = 0x0;

    if ((imageSize % sectorSize) != 0) {
        numSectors = (imageSize / sectorSize) + 1;
    } else {
        numSectors = imageSize / sectorSize;
    }
    return numSectors;
}

/**
 * Determines the address that is the end of this sector (exclusive - so it's really the start of the next sector.)
 */
uint32_t EndOfFlashSector(flash_device_t device, uint32_t address)
{
    uint32_t end;
    uint16_t sector = address / SERIAL_FLASH_SECTOR_SIZE;

    end = (sector+1) * SERIAL_FLASH_SECTOR_SIZE;
    return end;
}

bool FLASH_CheckValidAddressRange(flash_device_t flashDeviceID, uint32_t startAddress, uint32_t length)
{
    uint32_t endAddress = startAddress + length - 1;

    if (startAddress < SERIAL_FLASH_START || endAddress >= SERIAL_FLASH_END) {
        return false;
    }
    return true;
}

bool FLASH_WriteProtectMemory(flash_device_t flashDeviceID, uint32_t startAddress, uint32_t length, bool protect)
{
    return true;
}

bool FLASH_EraseMemory(flash_device_t flashDeviceID, uint32_t startAddress, uint32_t length)
{
    uint32_t startSector = 0, numSectors = 0;
    int eraseCounter = 0, rc = 0;

    if (FLASH_CheckValidAddressRange(flashDeviceID, startAddress, length) != true) {
        return false;
    }

    startSector = startAddress/SERIAL_FLASH_SECTOR_SIZE;
    numSectors = FLASH_SectorsMask(length, SERIAL_FLASH_SECTOR_SIZE);
    xt_rsil(15);
    for (eraseCounter = 0, rc = 0; (eraseCounter < numSectors)&&(rc == 0); eraseCounter++) {
        rc = spi_flash_erase_sector(startSector + eraseCounter);
    }
    xt_rsil(0);
    return rc == 0;
}

bool FLASH_WriteMemory(flash_device_t flashDeviceID, uint32_t startAddress, uint32_t *data, uint32_t length)
{
    xt_rsil(15);
    int rc = spi_flash_write(startAddress, data, length);
    xt_rsil(0);
    return rc == 0;
}

bool FLASH_ReadMemory(flash_device_t flashDeviceID, uint32_t startAddress, uint32_t *data, uint32_t length)
{
    xt_rsil(15);
    int rc = spi_flash_read(startAddress, data, length);
    xt_rsil(0);
    return rc == 0;
}

bool FLASH_CheckCopyMemory(flash_device_t sourceDeviceID, uint32_t sourceAddress,
        flash_device_t destinationDeviceID, uint32_t destinationAddress,
        uint32_t length, uint8_t module_function, uint8_t flags)
{
    return true;
}

bool CopyFlashBlock(flash_device_t sourceDeviceID, uint32_t sourceAddress, flash_device_t destinationDeviceID, uint32_t destinationAddress, uint32_t length)
{
    return true;
}

bool FLASH_CopyMemory(flash_device_t sourceDeviceID, uint32_t sourceAddress,
        flash_device_t destinationDeviceID, uint32_t destinationAddress,
        uint32_t length, uint8_t module_function, uint8_t flags)
{
    return true;
}

bool FLASH_CompareMemory(flash_device_t sourceDeviceID, uint32_t sourceAddress,
        flash_device_t destinationDeviceID, uint32_t destinationAddress,
        uint32_t length)
{
    return true;
}

void FLASH_WriteProtection_Enable(uint32_t FLASH_WRP_Sectors)
{

}

void FLASH_WriteProtection_Disable(uint32_t FLASH_WRP_Sectors)
{

}


void FLASH_Erase(void)
{

}

bool FLASH_Backup(uint32_t FLASH_Address)
{
    return true;
}

bool FLASH_Restore(uint32_t FLASH_Address)
{
    //CRC verification Disabled by default
    return true;
}

bool FLASH_Restore_Bootloader(uint32_t FLASH_Address)
{
    //CRC verification Disabled by default
    return true;
}

void FLASH_Begin(uint32_t FLASH_Address, uint32_t imageSize)
{

}

int FLASH_Update(const uint8_t *pBuffer, uint32_t address, uint32_t bufferSize)
{
    return -1;
}

void FLASH_End(void)
{

}
