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
static inline uint16_t InternalSectorToWriteProtect(uint32_t startAddress)
{
    uint16_t OB_WRP_Sector;
    OB_WRP_Sector = 1<<((startAddress-INTERNAL_FLASH_START)/INTERNAL_FLASH_SECTOR_SIZE);
    return OB_WRP_Sector;
}

inline static uint16_t InternalPageToErase(uint32_t startAddress)
{
    uint16_t flashPage = ((startAddress-INTERNAL_FLASH_START)/INTERNAL_FLASH_PAGE_SIZE);
    return flashPage;
}

uint16_t FLASH_SectorToWriteProtect(flash_device_t device, uint32_t startAddress)
{
    uint16_t sector = 0;
    if (device==FLASH_INTERNAL)
        sector = InternalSectorToWriteProtect(startAddress);
    return sector;
}

uint16_t FLASH_PageToErase(flash_device_t device, uint32_t startAddress)
{
    uint16_t sector = 0xFFFF;
    if (device==FLASH_INTERNAL)
        sector = InternalPageToErase(startAddress);
    return sector;
}

/**
 * Determines the address that is the end of this sector (exclusive - so it's really the start of the next sector.)
 */
uint32_t EndOfFlashPage(flash_device_t device, uint32_t address)
{
    uint32_t end;

    if (device==FLASH_INTERNAL)
    {
        uint16_t page = (address-INTERNAL_FLASH_START) / INTERNAL_FLASH_PAGE_SIZE;
        end = (page+1) * INTERNAL_FLASH_PAGE_SIZE;
    }
#ifdef USE_SERIAL_FLASH
    else if (device==FLASH_SERIAL)
    {
        uint16_t sector = address / sFLASH_PAGESIZE;
        end = (sector+1) * sFLASH_PAGESIZE;
    }
#endif
    else
        end = 0;
    return end;
}

bool FLASH_CheckValidAddressRange(flash_device_t flashDeviceID, uint32_t startAddress, uint32_t length)
{
    uint32_t endAddress = startAddress + length - 1;

    if (flashDeviceID == FLASH_INTERNAL)
    {
        return startAddress>=INTERNAL_FLASH_START && endAddress<=INTERNAL_FLASH_END;
    }
    else if (flashDeviceID == FLASH_SERIAL)
    {
#ifdef USE_SERIAL_FLASH
        if (startAddress < 0x4000 || endAddress >= 0x100000)
        {
            return false;
        }
#else
        return false;
#endif
    }
    else
    {
        return false;   //Invalid FLASH ID
    }

    return true;
}

bool FLASH_WriteProtectMemory(flash_device_t flashDeviceID, uint32_t startAddress, uint32_t length, bool protect)
{
    if (FLASH_CheckValidAddressRange(flashDeviceID, startAddress, length) != true)
    {
        return false;
    }

    if (flashDeviceID == FLASH_INTERNAL)
    {
        /* Get the first OB_WRP_Sector */
        uint16_t OB_WRP_Sector = InternalSectorToWriteProtect(startAddress);
        uint16_t end = InternalSectorToWriteProtect(startAddress+length-1)<<1;

        if (OB_WRP_Sector < OB_WRP_PAGES124TO127)
        {
            return false;
        }

        while (!(OB_WRP_Sector & end))
        {
            if (protect)
            {
                FLASH_WriteProtection_Enable(OB_WRP_Sector);
            }
            else
            {
                FLASH_WriteProtection_Disable(OB_WRP_Sector);
            }
            OB_WRP_Sector <<= 1;
        }

        return true;
    }

    return false;
}

bool FLASH_EraseMemory(flash_device_t flashDeviceID, uint32_t startAddress, uint32_t length)
{
    uint32_t numPages = 0;

    if (FLASH_CheckValidAddressRange(flashDeviceID, startAddress, length) != true)
    {
        return false;
    }

    if (flashDeviceID == FLASH_INTERNAL)
    {
        /* Check which sector has to be erased */
        uint16_t flashPage = InternalPageToErase(startAddress);

        if (flashPage > (INTERNAL_FLASH_SIZE/INTERNAL_FLASH_PAGE_SIZE)-1)
        {
            return false;
        }

        /* Disable memory write protection if any */
        FLASH_WriteProtectMemory(FLASH_INTERNAL, startAddress, length, false);

        /* Unlock the Flash Program Erase Controller */
        HAL_FLASH_Unlock();

        /* Define the number of Internal Flash pages to be erased */
        numPages = FLASH_PagesMask(length, INTERNAL_FLASH_PAGE_SIZE);

        uint32_t PAGEError = 0;
        FLASH_EraseInitTypeDef EraseInitStruct;
        EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
        EraseInitStruct.PageAddress = startAddress&(INTERNAL_FLASH_PAGE_SIZE-1);
        EraseInitStruct.NbPages     = numPages;
        if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
        {
            return false;
        }

        /* Locks the FLASH Program Erase Controller */
        HAL_FLASH_Lock();

        return true;
    }
    else if (flashDeviceID == FLASH_SERIAL)
    {
#ifdef USE_SERIAL_FLASH
        /* Initialize SPI Flash */
        sFLASH_Init();

        /* Define the number of External Flash pages to be erased */
        numPages = FLASH_PagesMask(length, sFLASH_PAGESIZE);

        /* Erase the SPI Flash pages */
        for (eraseCounter = 0; (eraseCounter < numPages); eraseCounter++)
        {
            sFLASH_EraseSector(startAddress + (sFLASH_PAGESIZE * eraseCounter));
        }

        /* Return Success */
        return true;
#endif
    }

    /* Return Failure */
    return false;
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

bool CopyFlashBlock(flash_device_t sourceDeviceID, uint32_t sourceAddress, flash_device_t destinationDeviceID, uint32_t destinationAddress, uint32_t length)
{
#ifdef USE_SERIAL_FLASH
    uint8_t serialFlashData[4];
#endif

    uint32_t endAddress = sourceAddress+length;

    if (!FLASH_EraseMemory(destinationDeviceID, destinationAddress, length))
    {
        return false;
    }

    if (destinationDeviceID == FLASH_INTERNAL)
    {
        /* Locks the internal flash program erase controller */
        HAL_FLASH_Unlock();
    }

    bool success = true;

    /* Program source to destination */
    while (sourceAddress < endAddress)
    {
        uint32_t internalFlashData = 0;

        if (sourceDeviceID == FLASH_INTERNAL)
        {
            /* Read data from internal flash source address */
            internalFlashData = (*(__IO uint32_t*) sourceAddress);
        }
#ifdef USE_SERIAL_FLASH
        else if (sourceDeviceID == FLASH_SERIAL)
        {
            /* Read data from serial flash source address */
            sFLASH_ReadBuffer(serialFlashData, sourceAddress, 4);
        }
#endif

        if (destinationDeviceID == FLASH_INTERNAL)
        {
#ifdef USE_SERIAL_FLASH
            if (sourceDeviceID == FLASH_SERIAL)
            {
                internalFlashData = (uint32_t)(serialFlashData[0] | (serialFlashData[1] << 8) | (serialFlashData[2] << 16) | (serialFlashData[3] << 24));
            }
#endif

            /* Program data to internal flash destination address */
            HAL_StatusTypeDef status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, destinationAddress, internalFlashData);

            /* If program operation fails, return Failure */
            if (status != HAL_OK)
            {
                success = false;
                break;
            }
        }
#ifdef USE_SERIAL_FLASH
        else if (destinationDeviceID == FLASH_SERIAL)
        {
            if (sourceDeviceID == FLASH_INTERNAL)
            {
                serialFlashData[0] = (uint8_t)(internalFlashData & 0xFF);
                serialFlashData[1] = (uint8_t)((internalFlashData & 0xFF00) >> 8);
                serialFlashData[2] = (uint8_t)((internalFlashData & 0xFF0000) >> 16);
                serialFlashData[3] = (uint8_t)((internalFlashData & 0xFF000000) >> 24);
            }

            /* Program data to serial flash destination address */
            sFLASH_WriteBuffer(serialFlashData, destinationAddress, 4);
        }
#endif

        sourceAddress += 4;
        destinationAddress += 4;
    }

    if (destinationDeviceID == FLASH_INTERNAL)
    {
        /* Locks the internal flash program erase controller */
        HAL_FLASH_Lock();
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

    if (sourceDeviceID == FLASH_SERIAL)
    {
#ifdef USE_SERIAL_FLASH
        /* Initialize SPI Flash */
        sFLASH_Init();
#endif
    }

    while (length)
    {
        uint32_t endBlockAddress = EndOfFlashPage(destinationDeviceID, destinationAddress);
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

    if (FLASH_CheckValidAddressRange(sourceDeviceID, sourceAddress, length) != true)
    {
        return false;
    }

    if (FLASH_CheckValidAddressRange(destinationDeviceID, destinationAddress, length) != true)
    {
        return false;
    }

    if (sourceDeviceID == FLASH_SERIAL || destinationDeviceID == FLASH_SERIAL)
    {
#ifdef USE_SERIAL_FLASH
        /* Initialize SPI Flash */
        sFLASH_Init();
#endif
    }

    /* Program source to destination */
    while (sourceAddress < endAddress)
    {
        uint32_t sourceDeviceData = 0;
        if (sourceDeviceID == FLASH_INTERNAL)
        {
            /* Read data from internal flash source address */
            sourceDeviceData = (*(__IO uint32_t*) sourceAddress);
        }
#ifdef USE_SERIAL_FLASH
        else if (sourceDeviceID == FLASH_SERIAL)
        {
            /* Read data from serial flash source address */
            sFLASH_ReadBuffer(serialFlashData, sourceAddress, 4);
            sourceDeviceData = (uint32_t)(serialFlashData[0] | (serialFlashData[1] << 8) | (serialFlashData[2] << 16) | (serialFlashData[3] << 24));
        }
#endif

        uint32_t destinationDeviceData = 0;
        if (destinationDeviceID == FLASH_INTERNAL)
        {
            /* Read data from internal flash destination address */
            destinationDeviceData = (*(__IO uint32_t*) destinationAddress);
        }
#ifdef USE_SERIAL_FLASH
        else if (destinationDeviceID == FLASH_SERIAL)
        {
            /* Read data from serial flash destination address */
            sFLASH_ReadBuffer(serialFlashData, destinationAddress, 4);
            destinationDeviceData = (uint32_t)(serialFlashData[0] | (serialFlashData[1] << 8) | (serialFlashData[2] << 16) | (serialFlashData[3] << 24));
        }
#endif

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

void FLASH_WriteProtection_Enable(uint32_t FLASH_WRP_Sectors)
{
    FLASH_OBProgramInitTypeDef OBInit;
    uint32_t SectorsWRPStatus = 0xFFF;

    HAL_FLASHEx_OBGetConfig(&OBInit);
    SectorsWRPStatus = OBInit.WRPPage & FLASH_WRP_Sectors;

    if (SectorsWRPStatus == FLASH_WRP_Sectors)
    {
        /* If FLASH_WRP_Sectors are not write protected, enable the write protection */

        /* Allow Access to option bytes sector */
        HAL_FLASH_OB_Unlock();

        /* Allow Access to Flash control registers and user Flash */
        HAL_FLASH_Unlock();

        /* Enable FLASH_WRP_Sectors write protection */
        OBInit.OptionType = OPTIONBYTE_WRP;
        OBInit.WRPState   = WRPSTATE_ENABLE;
        OBInit.WRPPage  = FLASH_WRP_Sectors;
        HAL_FLASHEx_OBProgram(&OBInit);

        /* Start the Option Bytes programming process */
        if (HAL_FLASH_OB_Launch() != HAL_OK)
        {
            //Error during Option Bytes programming process
        }

        /* Prevent Access to option bytes sector */
        HAL_FLASH_OB_Lock();

        /* Disable the Flash option control register access (recommended to protect
           the option Bytes against possible unwanted operations) */
        HAL_FLASH_Lock();

        /* Get FLASH_WRP_Sectors write protection status */
        HAL_FLASHEx_OBGetConfig(&OBInit);
        SectorsWRPStatus = OBInit.WRPPage & FLASH_WRP_Sectors;

        /* Check if FLASH_WRP_Sectors are write protected */
        if (SectorsWRPStatus != FLASH_WRP_Sectors)
        {
        }
    }
}

void FLASH_WriteProtection_Disable(uint32_t FLASH_WRP_Sectors)
{
    FLASH_OBProgramInitTypeDef OBInit;
    uint32_t SectorsWRPStatus = 0xFFF;

    /* Get FLASH_WRP_Sectors write protection status */
    HAL_FLASHEx_OBGetConfig(&OBInit);
    SectorsWRPStatus = OBInit.WRPPage & FLASH_WRP_Sectors;

    if (SectorsWRPStatus != FLASH_WRP_Sectors)
    {
        /* If FLASH_WRP_Sectors are write protected, disable the write protection */

        /* Allow Access to option bytes sector */
        HAL_FLASH_OB_Unlock();

        /* Allow Access to Flash control registers and user Flash */
        HAL_FLASH_Unlock();

        /* Disable FLASH_WRP_Sectors write protection */
        OBInit.OptionType = OPTIONBYTE_WRP;
        OBInit.WRPState   = WRPSTATE_DISABLE;
        OBInit.WRPPage  = FLASH_WRP_Sectors;
        HAL_FLASHEx_OBProgram(&OBInit);

        /* Start the Option Bytes programming process */
        if (HAL_FLASH_OB_Launch() != HAL_OK)
        {
            /* User can add here some code to deal with this error */
            while (1)
            {
            }
        }

        /* Prevent Access to option bytes sector */
        HAL_FLASH_OB_Lock();

        /* Disable the Flash option control register access (recommended to protect
           the option Bytes against possible unwanted operations) */
        HAL_FLASH_Lock();

        /* Get FLASH_WRP_Sectors write protection status */
        HAL_FLASHEx_OBGetConfig(&OBInit);
        SectorsWRPStatus = OBInit.WRPPage & FLASH_WRP_Sectors;

        /* Check if FLASH_WRP_Sectors write protection is disabled */
        if (SectorsWRPStatus == FLASH_WRP_Sectors)
        {
        }
    }
}

void FLASH_Erase(void)
{
    // This is too dangerous.
    //FLASH_EraseMemory(FLASH_INTERNAL, CORE_FW_ADDRESS, FIRMWARE_IMAGE_SIZE);
}

bool FLASH_Backup(uint32_t FLASH_Address)
{
#ifdef USE_SERIAL_FLASH
    FLASH_CopyMemory(FLASH_INTERNAL, CORE_FW_ADDRESS, FLASH_SERIAL, FLASH_Address, FIRMWARE_IMAGE_SIZE, 0, 0);
#else
    //Don't have enough space in Internal Flash to save a Backup copy of the firmware
#endif
}

bool FLASH_Restore(uint32_t FLASH_Address)
{
#ifdef USE_SERIAL_FLASH
    //CRC verification Disabled by default
    FLASH_CopyMemory(FLASH_SERIAL, FLASH_Address, FLASH_INTERNAL, CORE_FW_ADDRESS, FIRMWARE_IMAGE_SIZE, 0, 0);
#else
    //commented below since FIRMWARE_IMAGE_SIZE != Actual factory firmware image size
    //FLASH_CopyMemory(FLASH_INTERNAL, FLASH_Address, FLASH_INTERNAL, USER_FIRMWARE_IMAGE_LOCATION, FIRMWARE_IMAGE_SIZE, true);
    //FLASH_AddToFactoryResetModuleSlot() is now called in HAL_Core_Config() in core_hal.c
#endif
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
#ifdef USE_SERIAL_FLASH
    FLASH_EraseMemory(FLASH_SERIAL, FLASH_Address, imageSize);
#else
    FLASH_EraseMemory(FLASH_INTERNAL, FLASH_Address, imageSize);
#endif
}

int FLASH_Update(const uint8_t *pBuffer, uint32_t address, uint32_t bufferSize)
{
#ifdef USE_SERIAL_FLASH
    const uint8_t *writeBuffer = pBuffer;
    uint8_t readBuffer[bufferSize];

    readBuffer[0] = writeBuffer[0]+1;       // ensure different
    int i;

    for (i=50; i-->0 && memcmp(writeBuffer, readBuffer, bufferSize); )
    {
        /* Write Data Buffer to SPI Flash memory */
        sFLASH_WriteBuffer(writeBuffer, address, bufferSize);

        /* Read Data Buffer from SPI Flash memory */
        sFLASH_ReadBuffer(readBuffer, address, bufferSize);
    }

    return !i;
#else
    uint32_t index = 0;

    /* Unlock the internal flash */
    HAL_FLASH_Unlock();

    /* Data received are Word multiple */
    for (index = 0; index < (bufferSize & 0xFFFC); index += 4)
    {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, *(uint32_t *)(pBuffer + index));
        address += 4;
    }

    if (bufferSize & 0x3) /* Not an aligned data */
    {
        char buf[4];
        memset(buf, 0xFF, 4);

        for (index = bufferSize&3; index -->0; )
        {
            buf[index] = pBuffer[ (bufferSize & 0xFFFC)+index ];
        }
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, *(uint32_t *)(pBuffer + index));
    }

    /* Lock the internal flash */
    HAL_FLASH_Lock();

    return 0;
#endif
}

void FLASH_End(void)
{

}
