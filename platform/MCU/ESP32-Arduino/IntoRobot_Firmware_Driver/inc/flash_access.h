#include "hw_config.h"
#include "flash_device_hal.h"

#ifdef __cplusplus
extern "C" {
#endif


/* MAL access layer for Internal/Serial Flash Routines */
//New routines specific for BM09/BM14 flash usage
bool FLASH_CheckValidAddressRange(flash_device_t flashDeviceID, uint32_t startAddress, uint32_t length);
bool FLASH_EraseMemory(flash_device_t flashDeviceID, uint32_t startAddress, uint32_t length);

/**
 * Determines if the memory copy can be performed.
 */
bool FLASH_CheckCopyMemory(flash_device_t sourceDeviceID, uint32_t sourceAddress,
                      flash_device_t destinationDeviceID, uint32_t destinationAddress,
                      uint32_t length, uint8_t module_function, uint8_t flags);

/**
 * @param validateDestinationAddress checks if the destination address corresponds with the start address in the module
 */
bool FLASH_CopyMemory(flash_device_t sourceDeviceID, uint32_t sourceAddress,
                      flash_device_t destinationDeviceID, uint32_t destinationAddress,
                      uint32_t length, uint8_t module_function, uint8_t flags);

bool FLASH_CompareMemory(flash_device_t sourceDeviceID, uint32_t sourceAddress,
                         flash_device_t destinationDeviceID, uint32_t destinationAddress,
                         uint32_t length);

void FLASH_Erase(void);
bool FLASH_Backup(uint32_t FLASH_Address);
bool FLASH_Restore(uint32_t FLASH_Address);
bool FLASH_Restore_Bootloader(uint32_t FLASH_Address);
void FLASH_Begin(uint32_t FLASH_Address, uint32_t imageSize);
int FLASH_Update(const uint8_t *pBuffer, uint32_t address, uint32_t bufferSize);
void FLASH_End(void);


#ifdef __cplusplus
}
#endif
