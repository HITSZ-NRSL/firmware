#if 0
// pull in the sources from the HAL. It's a bit of a hack, but is simpler than trying to link the
// full hal library.
#include "../src/neutron/params_hal.cpp"

#else

#include <string.h>
#include <stdio.h>
#include "params_impl.h"
#include "params_hal.h"
#include "eeprom_hal.h"
#include "flash_map.h"
#include "flash_storage_impl.h"
#include "intorobot_macros.h"
#include "service_debug.h"

#define EEPROM_BOOT_PARAMS_MAX_SIZE                        (512)    //参数区大小

boot_params_t intorobot_boot_params;         //bootloader参数
system_params_t intorobot_system_params;     //设备参数

/*初始化bootloader参数区*/
void init_boot_params(boot_params_t *pboot_params) {
    memset((uint8_t *)pboot_params, 0, sizeof(boot_params_t));
    pboot_params->header = BOOT_PARAMS_HEADER;
}

/*初始化系统参数区*/
void init_system_params(system_params_t *psystem_params) {
    memset((uint8_t *)psystem_params, 0, sizeof(system_params_t));
    psystem_params->header = SYSTEM_PARAMS_HEADER;
    psystem_params->config_flag = 1;
    psystem_params->zone = 8;
}

/*初始化系统参数区 保留密钥参数*/
void init_fac_system_params(system_params_t *psystem_params) {
    uint8_t  at_mode;                  // 是否已经灌装密钥  0:未灌装 1:已经灌装
    uint8_t  device_id[52]={0};        // 设备序列号
    uint8_t  access_token[52]={0};     // 设备access_token

    at_mode = psystem_params->at_mode;
    memcpy(device_id, psystem_params->device_id, sizeof(psystem_params->device_id));
    memcpy(access_token, psystem_params->access_token, sizeof(psystem_params->access_token));

    init_system_params(psystem_params);

    psystem_params->at_mode = at_mode;
    memcpy(psystem_params->device_id, device_id, sizeof(psystem_params->device_id));
    memcpy(psystem_params->access_token, access_token, sizeof(psystem_params->access_token));
}

void save_boot_params(boot_params_t *pboot_params);
/*
 * 读取bootloader参数区
 * */
void read_boot_params(boot_params_t *pboot_params) {
    uint32_t len = sizeof(boot_params_t);
    uint32_t address = HAL_EEPROM_Length() - EEPROM_BOOT_PARAMS_MAX_SIZE;
    uint8_t *pboot = (uint8_t *)pboot_params;

    memset(pboot, 0, len);
    if(len > EEPROM_BOOT_PARAMS_MAX_SIZE) {
        return;
    }
    for (int num = 0; num<len; num++) {
        pboot[num] = HAL_EEPROM_Read(address+num);
    }
}

/*
 * 保存bootloader参数区
 * */
void save_boot_params(boot_params_t *pboot_params) {
    uint32_t len = sizeof(boot_params_t);
    uint32_t address = HAL_EEPROM_Length() - EEPROM_BOOT_PARAMS_MAX_SIZE;
    uint8_t *pboot = (uint8_t *)pboot_params;

    if(len > EEPROM_BOOT_PARAMS_MAX_SIZE) {
        return;
    }

    for (int num = 0; num<len; num++) {
        HAL_EEPROM_Write(address+num, pboot[num]);
    }
}

void save_system_params(system_params_t *psystem_params);
/*
 * 加载系统参数区
 * */
void read_system_params(system_params_t *psystem_params) {
    uint32_t len = sizeof(system_params_t);
    InternalFlashStore flashStore;

    memset(psystem_params, 0, sizeof(system_params_t));
    if(len > (SYSTEM_PARAMS_END_ADDR - SYSTEM_PARAMS_START_ADDR)) {
        return;
    }
    flashStore.read(SYSTEM_PARAMS_START_ADDR, psystem_params, len);
    for (int num = 0; num<len; num++) {
    }
}

/*
 * 保存系统参数区
 * */
void save_system_params(system_params_t *psystem_params) {
    uint32_t len = sizeof(system_params_t);
    InternalFlashStore flashStore;

    if(len > (SYSTEM_PARAMS_END_ADDR - SYSTEM_PARAMS_START_ADDR)) {
        return;
    }
    flashStore.eraseSector(SYSTEM_PARAMS_START_ADDR);
    flashStore.write(SYSTEM_PARAMS_START_ADDR, psystem_params, len);
}

/*
 * 恢复默认参数  不保留密钥信息
 * */
void HAL_PARAMS_Init_All_System_Params(void) {
    init_system_params(&intorobot_system_params);
    save_system_params(&intorobot_system_params);
}

/*
 * 恢复默认参数  保留密钥信息
 * */

void HAL_PARAMS_Init_Fac_System_Params(void) {
    init_fac_system_params(&intorobot_system_params);
    save_system_params(&intorobot_system_params);
}
/*
 * 保存系统参数区
 * */

void HAL_PARAMS_Init_Boot_Params(void) {
    init_boot_params(&intorobot_boot_params);
    save_boot_params(&intorobot_boot_params);
}

/*
 * 读取bootloader参数区
 * */
void HAL_PARAMS_Load_Boot_Params(void) {
    read_boot_params(&intorobot_boot_params);
    if( BOOT_PARAMS_HEADER != intorobot_boot_params.header ) {
        //擦除eeprom区域 并初始化
        InternalFlashStore flashStore;
        flashStore.eraseSector(EEPROM_START_ADDR);
        flashStore.eraseSector(EEPROM_START_ADDR + 0x4000);
        HAL_EEPROM_Init();
        HAL_PARAMS_Init_Boot_Params();
    }
}

/*
 * 读取系统参数区
 * */
void HAL_PARAMS_Load_System_Params(void) {
    read_system_params(&intorobot_system_params);
    if( SYSTEM_PARAMS_HEADER != intorobot_system_params.header ) {
        HAL_PARAMS_Init_All_System_Params();
    }
}

/*
 *  保存参数区
 * */
void HAL_PARAMS_Save_Params(void) {
    save_boot_params(&intorobot_boot_params);
    save_system_params(&intorobot_system_params);
}

/*
 * 读取bootloader版本号
 * */
uint32_t HAL_PARAMS_Get_Boot_boot_version(void) {
    return intorobot_boot_params.boot_version;
}

/*
 * 保存bootloader版本号
 * */
int HAL_PARAMS_Set_Boot_boot_version(uint32_t version) {
    intorobot_boot_params.boot_version = version;
    return 0;
}

/*
 * 读取设置启动标志
 * */
BOOT_FLAG_TypeDef HAL_PARAMS_Get_Boot_boot_flag(void) {
    return (BOOT_FLAG_TypeDef)intorobot_boot_params.boot_flag;
}

/*
 * 保存设置启动标志
 * */
int HAL_PARAMS_Set_Boot_boot_flag(BOOT_FLAG_TypeDef flag) {
    intorobot_boot_params.boot_flag = flag;
    return 0;
}

/*
 * 读取设置是否恢复默认参数标志
 * */
INITPARAM_FLAG_TypeDef HAL_PARAMS_Get_Boot_initparam_flag(void) {
    return (INITPARAM_FLAG_TypeDef)intorobot_boot_params.initparam_flag;
}

/*
 * 保存设置是否恢复默认参数标志
 * */
int HAL_PARAMS_Set_Boot_initparam_flag(INITPARAM_FLAG_TypeDef flag) {
    intorobot_boot_params.initparam_flag = flag;
    return 0;
}

#endif
