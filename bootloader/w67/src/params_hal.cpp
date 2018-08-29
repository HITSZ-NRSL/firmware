#if 0
// pull in the sources from the HAL. It's a bit of a hack, but is simpler than trying to link the
// full hal library.
#include "../src/esp8266-share/params_hal.cpp"

#else

#include <string.h>
#include <stdio.h>
#include "params_impl.h"
#include "params_hal.h"
#include "memory_hal.h"
#include "flash_map.h"
#include "intorobot_macros.h"
#include "molmc_log.h"


hal_boot_params_t intorobot_boot_params;         //bootloader参数
hal_system_params_t intorobot_system_params;     //设备参数


/*初始化bootloader参数区*/
void init_boot_params(hal_boot_params_t *pboot_params) {
    memset((uint8_t *)pboot_params, 0, sizeof(hal_boot_params_t));
    pboot_params->header = BOOT_PARAMS_HEADER;
}

/*初始化系统参数区*/
void init_system_params(hal_system_params_t *psystem_params) {
    memset((uint8_t *)psystem_params, 0, sizeof(hal_system_params_t));
    psystem_params->header = SYSTEM_PARAMS_HEADER;
    psystem_params->config_flag = 1;
    psystem_params->zone = 8;
}

/*初始化系统参数区 保留密钥参数*/
void init_fac_system_params(hal_system_params_t *psystem_params) {
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

void save_boot_params(hal_boot_params_t *pboot_params);
/*
 * 读取bootloader参数区
 * */
void read_boot_params(hal_boot_params_t *pboot_params) {
    uint32_t len = sizeof(hal_boot_params_t);

    memset(pboot_params, 0, sizeof(hal_boot_params_t));
    if(len > (BOOT_PARAMS_END_ADDR-BOOT_PARAMS_START_ADDR)) {
        return;
    }
    HAL_FLASH_Interminal_Read(BOOT_PARAMS_START_ADDR, (uint32_t *)pboot_params, len);
}

/*
 * 保存bootloader参数区
 * */
void save_boot_params(hal_boot_params_t *pboot_params) {
    uint32_t len = sizeof(hal_boot_params_t);

    if(len > (BOOT_PARAMS_END_ADDR - BOOT_PARAMS_START_ADDR)) {
        return;
    }
    HAL_FLASH_Interminal_Erase(HAL_FLASH_Interminal_Get_Sector(BOOT_PARAMS_START_ADDR));
    HAL_FLASH_Interminal_Write(BOOT_PARAMS_START_ADDR, (uint32_t *)pboot_params, len);
}

void save_system_params(hal_system_params_t *psystem_params);
/*
 * 加载系统参数区
 * */
void read_system_params(hal_system_params_t *psystem_params) {
    uint32_t len = sizeof(hal_system_params_t);

    memset(psystem_params, 0, sizeof(hal_system_params_t));
    if(len > (SYSTEM_PARAMS_END_ADDR-SYSTEM_PARAMS_START_ADDR)) {
        return;
    }
    HAL_FLASH_Interminal_Read(SYSTEM_PARAMS_START_ADDR, (uint32_t *)psystem_params, len);
}

/*
 * 保存系统参数区
 * */
void save_system_params(hal_system_params_t *psystem_params) {
    uint32_t len = sizeof(hal_system_params_t);

    if(len > (SYSTEM_PARAMS_END_ADDR - SYSTEM_PARAMS_START_ADDR)) {
        return;
    }
    HAL_FLASH_Interminal_Erase(HAL_FLASH_Interminal_Get_Sector(SYSTEM_PARAMS_START_ADDR));
    HAL_FLASH_Interminal_Write(SYSTEM_PARAMS_START_ADDR, (uint32_t *)psystem_params, len);
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
/********************************************************************************
 *  添加参数
 ********************************************************************************/
extern "C" {
    uint32_t HAL_PARAMS_Get_Boot_ota_app_size(void);
    int HAL_PARAMS_Set_Boot_ota_app_size(uint32_t size);
    uint32_t HAL_PARAMS_Get_Boot_def_app_size(void);
    int HAL_PARAMS_Set_Boot_def_app_size(uint32_t size);
    uint32_t HAL_PARAMS_Get_Boot_boot_size(void);
    int HAL_PARAMS_Set_Boot_boot_size(uint32_t size);
}

/*
 * 读取ota文件大小
 * */
uint32_t HAL_PARAMS_Get_Boot_ota_app_size(void) {
    return intorobot_boot_params.ota_app_size;
}

/*
 * 保存ota文件大小
 * */
int HAL_PARAMS_Set_Boot_ota_app_size(uint32_t size) {
    intorobot_boot_params.ota_app_size = size;
    return 0;
}

/*
 * 读取默认应用文件大小
 * */
uint32_t HAL_PARAMS_Get_Boot_def_app_size(void) {
    return intorobot_boot_params.def_app_size;
}

/*
 * 保存默认应用文件大小
 * */
int HAL_PARAMS_Set_Boot_def_app_size(uint32_t size) {
    intorobot_boot_params.def_app_size = size;
    return 0;
}

/*
 * 读取升级boot文件大小
 * */
uint32_t HAL_PARAMS_Get_Boot_boot_size(void) {
    return intorobot_boot_params.boot_size;
}

/*
 * 保存升级boot文件大小
 * */
int HAL_PARAMS_Set_Boot_boot_size(uint32_t size) {
    intorobot_boot_params.boot_size = size;
    return 0;
}

#endif
