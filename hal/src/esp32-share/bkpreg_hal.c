#include "bkpreg_hal.h"
#include "hw_config.h"

#define RTC_BKP_DR0                       ((uint32_t)0x00000000U)
#define RTC_BKP_DR1                       ((uint32_t)0x00000001U)
#define RTC_BKP_DR2                       ((uint32_t)0x00000002U)
#define RTC_BKP_DR3                       ((uint32_t)0x00000003U)
#define RTC_BKP_DR4                       ((uint32_t)0x00000004U)
#define RTC_BKP_DR5                       ((uint32_t)0x00000005U)
#define RTC_BKP_DR6                       ((uint32_t)0x00000006U)
#define RTC_BKP_DR7                       ((uint32_t)0x00000007U)
#define RTC_BKP_DR8                       ((uint32_t)0x00000008U)
#define RTC_BKP_DR9                       ((uint32_t)0x00000009U)
#define RTC_BKP_DR10                      ((uint32_t)0x0000000AU)

#define RTC_MEM ((volatile uint32_t*)0x50000000)

int32_t HAL_Core_Backup_Register(uint32_t BKP_DR)
{
    switch (BKP_DR) {
        case BKP_DR_01:
            return RTC_BKP_DR1;
            break;
        case BKP_DR_02:
            return RTC_BKP_DR2;
            break;
        case BKP_DR_03:
            return RTC_BKP_DR3;
            break;
        case BKP_DR_04:
            return RTC_BKP_DR4;
            break;
        case BKP_DR_05:
            return RTC_BKP_DR5;
            break;
        case BKP_DR_06:
            return RTC_BKP_DR6;
            break;
        case BKP_DR_07:
            return RTC_BKP_DR7;
            break;
        case BKP_DR_08:
            return RTC_BKP_DR8;
            break;
        case BKP_DR_09:
            return RTC_BKP_DR9;
            break;
        case BKP_DR_10:
            return RTC_BKP_DR10;
            break;
        default:
            break;
    }
    return -1;
}

void HAL_Core_Write_Backup_Register(uint32_t BKP_DR, uint32_t Data)
{
    uint32_t BKP_DR_Address = HAL_Core_Backup_Register(BKP_DR);
    if (BKP_DR_Address != -1) {
        RTC_MEM[BKP_DR_Address] = Data;
    }
}

uint32_t HAL_Core_Read_Backup_Register(uint32_t BKP_DR)
{
    uint32_t BKP_DR_Address = HAL_Core_Backup_Register(BKP_DR);
    if (BKP_DR_Address != -1) {
        return RTC_MEM[BKP_DR_Address];
    }
    return 0xFFFFFFFF;
}

