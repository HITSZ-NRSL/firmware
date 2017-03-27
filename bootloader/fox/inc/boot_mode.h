/* Copyright (c) 2015 Ivan Grokhotkov. All rights reserved.
 * This file is part of eboot bootloader.
 *
 * Redistribution and use is permitted according to the conditions of the
 * 3-clause BSD license to be found in the LICENSE file.
 */

#ifndef BOOT_MODE_H
#define BOOT_MODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>


typedef enum
{
    DEFAULT_FIRWARE = 0, OTA_FIRWARE = 1
} Firmware_TypeDef;


void start_app(void);
void Enter_Default_RESTORE_Mode(void);
void Enter_Serail_Com_Mode(void);
void Enter_Factory_RESTORE_Mode(void);
void Enter_OTA_Update_Mode(void);
void Enter_DFU_Mode(void);

#ifdef __cplusplus
}
#endif


#endif //BOOT_MODE_H
