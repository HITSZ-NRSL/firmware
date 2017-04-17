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
#include <string.h>
#include "hw_config.h"
#include "flash_mal.h"

/* Private functions ---------------------------------------------------------*/

/**
 * Determines the address that is the end of this sector (exclusive - so it's really the start of the next sector.)
 */
uint32_t EndOfFlashSector(flash_device_t device, uint32_t address)
{
    uint32_t end;

    if (device==FLASH_SERIAL) {
        uint16_t sector = address / sFLASH_PAGESIZE;
        end = (sector+1) * sFLASH_PAGESIZE;
    } else {
        end = 0;
    }

    return end;
}

bool FLASH_CheckValidAddressRange(flash_device_t flashDeviceID, uint32_t startAddress, uint32_t length)
{
    uint32_t endAddress = startAddress + length - 1;

    if (flashDeviceID == FLASH_SERIAL) {
        if (startAddress < 0x20000 || endAddress >= 0x400000) {
            return false;
        }
    } else {
        return false;
    }

    return true;
}

bool FLASH_EraseMemory(flash_device_t flashDeviceID, uint32_t startAddress, uint32_t length)
{
    uint32_t numPages = 0;
    int erasePage = 0;

    /* Define the number of External Flash pages to be erased */
    numPages = FLASH_PagesMask(length, sFLASH_PAGESIZE);

    uint32_t startPage = startAddress/sFLASH_PAGESIZE;
    /* Erase the SPI Flash pages */
    for (erasePage = startPage; erasePage < (startPage + numPages); erasePage++)
    {
        spi_flash_erase_sector(erasePage);
    }

    /* Return Success */
    return true;
}

bool FLASH_CheckCopyMemory(flash_device_t sourceDeviceID, uint32_t sourceAddress,
        flash_device_t destinationDeviceID, uint32_t destinationAddress,
        uint32_t length, uint8_t module_function, uint8_t flags)
{
    if (!FLASH_CheckValidAddressRange(sourceDeviceID, sourceAddress, length))
    {
        return false;
    }

    if (!FLASH_CheckValidAddressRange(destinationDeviceID, destinationAddress, length))
    {
        return false;
    }

    return true;
}

bool CopyFlashBlock(flash_device_t sourceDeviceID, uint32_t sourceAddress,
        flash_device_t destinationDeviceID, uint32_t destinationAddress,
        uint32_t length)
{
    uint8_t serialFlashData[4];

    uint32_t endAddress = sourceAddress+length;

    if (!FLASH_EraseMemory(destinationDeviceID, destinationAddress, length))
    {
        return false;
    }

    bool success = true;

    /* Program source to destination */
    while (sourceAddress < endAddress)
    {
        uint32_t internalFlashData = 0;

        /* Read data from serial flash source address */
        spi_flash_read(sourceAddress, serialFlashData, 4);

        /* Program data to serial flash destination address */
        spi_flash_write(destinationAddress, serialFlashData, 4);

        sourceAddress += 4;
        destinationAddress += 4;
    }

    return success;
}

bool FLASH_CopyMemory(flash_device_t sourceDeviceID, uint32_t sourceAddress,
        flash_device_t destinationDeviceID, uint32_t destinationAddress,
        uint32_t length, uint8_t module_function, uint8_t flags)
{
    if (!FLASH_CheckCopyMemory(sourceDeviceID, sourceAddress, destinationDeviceID, destinationAddress, length, module_function, flags))
    {
        return false;
    }

    while (length)
    {
        uint32_t endBlockAddress = EndOfFlashSector(destinationDeviceID, destinationAddress);
        uint32_t blockLength = endBlockAddress-destinationAddress;
        if (blockLength>length)
            blockLength = length;
        if (!CopyFlashBlock(sourceDeviceID, sourceAddress, destinationDeviceID, destinationAddress, blockLength))
            return false;
        length -= blockLength;
        sourceAddress += blockLength;
        destinationAddress += blockLength;
    }
    return true;
}

bool FLASH_CompareMemory(flash_device_t sourceDeviceID, uint32_t sourceAddress,
        flash_device_t destinationDeviceID, uint32_t destinationAddress,
        uint32_t length)
{
    uint32_t endAddress = sourceAddress + length;
    uint8_t serialFlashData[4];

    if (FLASH_CheckValidAddressRange(sourceDeviceID, sourceAddress, length) != true)
    {
        return false;
    }

    if (FLASH_CheckValidAddressRange(destinationDeviceID, destinationAddress, length) != true)
    {
        return false;
    }

    /* Program source to destination */
    while (sourceAddress < endAddress)
    {
        uint32_t sourceDeviceData = 0;

        /* Read data from serial flash source address */
        spi_flash_read(sourceAddress, serialFlashData, 4);
        sourceDeviceData = (uint32_t)(serialFlashData[0] | (serialFlashData[1] << 8) | (serialFlashData[2] << 16) | (serialFlashData[3] << 24));

        uint32_t destinationDeviceData = 0;
        /* Read data from serial flash destination address */
        spi_flash_read(destinationAddress, serialFlashData, 4);
        destinationDeviceData = (uint32_t)(serialFlashData[0] | (serialFlashData[1] << 8) | (serialFlashData[2] << 16) | (serialFlashData[3] << 24));

        if (sourceDeviceData != destinationDeviceData)
        {
            /* Failed comparison check */
            return false;
        }

        sourceAddress += 4;
        destinationAddress += 4;
    }

    /* Passed comparison check */
    return true;
}

void FLASH_Erase(void)
{
    // This is too dangerous.
    //FLASH_EraseMemory(FLASH_SERIAL, CORE_FW_ADDRESS, FIRMWARE_IMAGE_SIZE);
}

bool FLASH_Backup(uint32_t FLASH_Address)
{
    FLASH_CopyMemory(FLASH_SERIAL, CORE_FW_ADDRESS, FLASH_SERIAL, FLASH_Address, FIRMWARE_IMAGE_SIZE, 0, 0);
}

bool FLASH_Restore(uint32_t FLASH_Address)
{
    FLASH_CopyMemory(FLASH_SERIAL, FLASH_Address, FLASH_SERIAL, CORE_FW_ADDRESS, FIRMWARE_IMAGE_SIZE, 0, 0);
}

bool FLASH_Restore_Bootloader(uint32_t FLASH_Address)
{
    //FLASH_CopyMemory(FLASH_SERIAL, FLASH_Address, FLASH_SERIAL, INTERNAL_FLASH_START, BOOTLOADER_IMAGE_SIZE, 0, 0);
}

uint32_t FLASH_PagesMask(uint32_t imageSize, uint32_t pageSize)
{
    //Calculate the number of flash pages that needs to be erased
    uint32_t numPages = 0x0;

    if ((imageSize % pageSize) != 0)
    {
        numPages = (imageSize / pageSize) + 1;
    }
    else
    {
        numPages = imageSize / pageSize;
    }

    return numPages;
}

void FLASH_Begin(uint32_t FLASH_Address, uint32_t imageSize)
{
    FLASH_EraseMemory(FLASH_SERIAL, FLASH_Address, imageSize);
}

int FLASH_Update(const uint8_t *pBuffer, uint32_t address, uint32_t bufferSize)
{
    if (spi_flash_erase_sector(address/sFLASH_PAGESIZE) != ESP_OK) {
        return 1;
    }
    if (spi_flash_write(address, (uint32_t*)pBuffer, bufferSize) != ESP_OK) {
        return 1;
    }
    return 0;
}

void FLASH_End(void)
{

}
