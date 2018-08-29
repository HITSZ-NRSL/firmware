#include "core_hal.h"
#include "hw_config.h"

extern RTC_HandleTypeDef RtcHandle;

int32_t HAL_Core_Backup_Register(uint32_t BKP_DR)
{
    return -1;
}

void HAL_Core_Write_Backup_Register(uint32_t BKP_DR, uint32_t Data)
{
}

uint32_t HAL_Core_Read_Backup_Register(uint32_t BKP_DR)
{
    return 0xFFFFFFFF;
}
