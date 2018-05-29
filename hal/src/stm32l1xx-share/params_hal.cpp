/**
 ******************************************************************************
  Copyright (c) 2013-2014 IntoRobot Team.  All right reserved.

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

#include <string.h>
#include <stdio.h>
#include "params_impl.h"
#include "params_hal.h"
#include "flash_map.h"
#include "flash_storage_impl.h"
#include "intorobot_macros.h"

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
    uint8_t  at_mode;
    uint8_t  deveui[20]={0}, appeui[20]={0}, appkey[36]={0}, devaddr[12]={0}, nwkskey[36]={0}, appskey[36]={0};

    switch(psystem_params->at_mode)
    {
        case 1:      //Activation By Personalization  //已经灌好密钥
            at_mode = psystem_params->at_mode;
            memcpy(deveui, psystem_params->deveui, sizeof(psystem_params->deveui));
            memcpy(devaddr, psystem_params->devaddr, sizeof(psystem_params->devaddr));
            memcpy(nwkskey, psystem_params->nwkskey, sizeof(psystem_params->nwkskey));
            memcpy(appskey, psystem_params->appskey, sizeof(psystem_params->appskey));
            init_system_params(psystem_params);
            psystem_params->at_mode = at_mode;
            memcpy(psystem_params->deveui, deveui, sizeof(psystem_params->deveui));
            memcpy(psystem_params->devaddr, devaddr, sizeof(psystem_params->devaddr));
            memcpy(psystem_params->nwkskey, nwkskey, sizeof(psystem_params->nwkskey));
            memcpy(psystem_params->appskey, appskey, sizeof(psystem_params->appskey));
            break;
        case 2:      //Over-The-Air Activation //灌装激活码  未激活
        case 3:      //灌装激活码 已激活
            memcpy(deveui, psystem_params->deveui, sizeof(psystem_params->deveui));
            memcpy(appeui, psystem_params->appeui, sizeof(psystem_params->appeui));
            memcpy(appkey, psystem_params->appkey, sizeof(psystem_params->appkey));
            init_system_params(psystem_params);
            psystem_params->at_mode = 2; //灌装激活码  未激活
            memcpy(psystem_params->deveui, deveui, sizeof(psystem_params->deveui));
            memcpy(psystem_params->appeui, appeui, sizeof(psystem_params->appeui));
            memcpy(psystem_params->appkey, appkey, sizeof(psystem_params->appkey));
            break;
        default:     //没有密钥信息
            init_system_params(psystem_params);
            break;
    }
}

void save_boot_params(hal_boot_params_t *pboot_params);
/*
 * 读取bootloader参数区
 * */
void read_boot_params(hal_boot_params_t *pboot_params) {
    uint32_t len = sizeof(hal_boot_params_t);
    InternalFlashStore flashStore;

    memset(pboot_params, 0, sizeof(hal_boot_params_t));
    if(len > (BOOT_PARAMS_END_ADDR - BOOT_PARAMS_START_ADDR)) {
        return;
    }
    flashStore.read(BOOT_PARAMS_START_ADDR, pboot_params, len);
}

/*
 * 保存bootloader参数区
 * */
void save_boot_params(hal_boot_params_t *pboot_params) {
    uint32_t len = sizeof(hal_boot_params_t);
    InternalFlashStore flashStore;

    if(len > (BOOT_PARAMS_END_ADDR - BOOT_PARAMS_START_ADDR)) {
        return;
    }
    flashStore.erase(BOOT_PARAMS_START_ADDR, len);
    flashStore.write(BOOT_PARAMS_START_ADDR, pboot_params, len);
}

void save_system_params(hal_system_params_t *psystem_params);
/*
 * 加载系统参数区
 * */
void read_system_params(hal_system_params_t *psystem_params) {
    uint32_t len = sizeof(hal_system_params_t);
    InternalFlashStore flashStore;

    memset(psystem_params, 0, sizeof(hal_system_params_t));
    if(len > (SYSTEM_PARAMS_END_ADDR - SYSTEM_PARAMS_START_ADDR)) {
        return;
    }
    flashStore.read(SYSTEM_PARAMS_START_ADDR, psystem_params, len);
}

/*
 * 保存系统参数区
 * */
void save_system_params(hal_system_params_t *psystem_params) {
    uint32_t len = sizeof(hal_system_params_t);
    InternalFlashStore flashStore;

    if(len > (SYSTEM_PARAMS_END_ADDR - SYSTEM_PARAMS_START_ADDR)) {
        return;
    }
    flashStore.erase(SYSTEM_PARAMS_START_ADDR, len);
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

/*
 * 获取设备识别号
 * */
uint16_t HAL_PARAMS_Get_System_device_id(char* buffer, uint16_t len) {
    uint16_t templen;

    if (buffer!=NULL && len>0) {
        templen = MIN(strlen(intorobot_system_params.deveui), len-1);
        memset(buffer, 0, len);
        memcpy(buffer, intorobot_system_params.deveui, templen);
        return templen;
    }
    return 0;
}

/*
 * 设置设备识别号
 * */
int HAL_PARAMS_Set_System_device_id(const char* buffer) {
    uint16_t templen;

    if (buffer!=NULL) {
        templen = MIN(sizeof(intorobot_system_params.deveui)-1, strlen(buffer));
        memset(intorobot_system_params.deveui, 0, sizeof(intorobot_system_params.deveui));
        memcpy(intorobot_system_params.deveui, buffer, templen);
        return 0;
    }
    return -1;
}

/*
 * 获取应用识别号
 * */
uint16_t HAL_PARAMS_Get_System_appeui(char* buffer, uint16_t len) {
    uint16_t templen;

    if (buffer!=NULL && len>0) {
        templen = MIN(strlen(intorobot_system_params.appeui), len-1);
        memset(buffer, 0, len);
        memcpy(buffer, intorobot_system_params.appeui, templen);
        return templen;
    }
    return 0;
}

/*
 * 设置应用识别号
 * */
int HAL_PARAMS_Set_System_appeui(const char* buffer) {
    uint16_t templen;

    if (buffer!=NULL) {
        templen = MIN(sizeof(intorobot_system_params.appeui)-1, strlen(buffer));
        memset(intorobot_system_params.appeui, 0, sizeof(intorobot_system_params.appeui));
        memcpy(intorobot_system_params.appeui, buffer, templen);
        return 0;
    }
    return -1;
}

/*
 * 获取应用密钥
 * */
uint16_t HAL_PARAMS_Get_System_appkey(char* buffer, uint16_t len) {
    uint16_t templen;

    if (buffer!=NULL && len>0) {
        templen = MIN(strlen(intorobot_system_params.appkey), len-1);
        memset(buffer, 0, len);
        memcpy(buffer, intorobot_system_params.appkey, templen);
        return templen;
    }
    return 0;
}

/*
 * 设置应用密钥
 * */
int HAL_PARAMS_Set_System_appkey(const char* buffer) {
    uint16_t templen;

    if (buffer!=NULL) {
        templen = MIN(sizeof(intorobot_system_params.appkey)-1, strlen(buffer));
        memset(intorobot_system_params.appkey, 0, sizeof(intorobot_system_params.appkey));
        memcpy(intorobot_system_params.appkey, buffer, templen);
        return 0;
    }
    return -1;
}

/*
 * 获取设备地址
 * */
uint16_t HAL_PARAMS_Get_System_devaddr(char* buffer, uint16_t len) {
    uint16_t templen;

    if (buffer!=NULL && len>0) {
        templen = MIN(strlen(intorobot_system_params.devaddr), len-1);
        memset(buffer, 0, len);
        memcpy(buffer, intorobot_system_params.devaddr, templen);
        return templen;
    }
    return 0;
}

/*
 * 设置设备地址
 * */
int HAL_PARAMS_Set_System_devaddr(const char* buffer) {
    uint16_t templen;

    if (buffer!=NULL) {
        templen = MIN(sizeof(intorobot_system_params.devaddr)-1, strlen(buffer));
        memset(intorobot_system_params.devaddr, 0, sizeof(intorobot_system_params.devaddr));
        memcpy(intorobot_system_params.devaddr, buffer, templen);
        return 0;
    }
    return -1;
}

/*
 * 获取网络会话密钥
 * */
uint16_t HAL_PARAMS_Get_System_nwkskey(char* buffer, uint16_t len) {
    uint16_t templen;

    if (buffer!=NULL && len>0) {
        templen = MIN(strlen(intorobot_system_params.nwkskey), len-1);
        memset(buffer, 0, len);
        memcpy(buffer, intorobot_system_params.nwkskey, templen);
        return templen;
    }
    return 0;
}

/*
 * 设置网络会话密钥
 * */
int HAL_PARAMS_Set_System_nwkskey(const char* buffer) {
    uint16_t templen;

    if (buffer!=NULL) {
        templen = MIN(sizeof(intorobot_system_params.nwkskey)-1, strlen(buffer));
        memset(intorobot_system_params.nwkskey, 0, sizeof(intorobot_system_params.nwkskey));
        memcpy(intorobot_system_params.nwkskey, buffer, templen);
        return 0;
    }
    return -1;
}

/*
 * 获取应用会话密钥
 * */
uint16_t HAL_PARAMS_Get_System_appskey(char* buffer, uint16_t len) {
    uint16_t templen;

    if (buffer!=NULL && len>0) {
        templen = MIN(strlen(intorobot_system_params.appskey), len-1);
        memset(buffer, 0, len);
        memcpy(buffer, intorobot_system_params.appskey, templen);
        return templen;
    }
    return 0;
}

/*
 * 设置应用会话密钥
 * */
int HAL_PARAMS_Set_System_appskey(const char* buffer) {
    uint16_t templen;

    if (buffer!=NULL) {
        templen = MIN(sizeof(intorobot_system_params.appskey)-1, strlen(buffer));
        memset(intorobot_system_params.appskey, 0, sizeof(intorobot_system_params.appskey));
        memcpy(intorobot_system_params.appskey, buffer, templen);
        return 0;
    }
    return -1;
}

/*
 * 读取at_mode标志
 * */
AT_MODE_FLAG_TypeDef HAL_PARAMS_Get_System_at_mode(void) {
    return (AT_MODE_FLAG_TypeDef)intorobot_system_params.at_mode;
}

/*
 * 保存at_mode标志
 * */
int HAL_PARAMS_Set_System_at_mode(AT_MODE_FLAG_TypeDef flag) {
    intorobot_system_params.at_mode = flag;
    return 0;
}

/*
 * 读取时区
 * */
float HAL_PARAMS_Get_System_zone(void) {
    return intorobot_system_params.zone;
}

/*
 * 保存时区
 * */
int HAL_PARAMS_Set_System_zone(float zone) {
    intorobot_system_params.zone = zone;
    return 0;
}

/*
 * 读取固件库版本号
 * */
uint16_t HAL_PARAMS_Get_System_fwlib_ver(char* buffer, uint16_t len) {
    uint16_t templen;

    if (buffer!=NULL && len>0) {
        templen = MIN(strlen(intorobot_system_params.fwlib_ver), len-1);
        memset(buffer, 0, len);
        memcpy(buffer, intorobot_system_params.fwlib_ver, templen);
        return templen;
    }
    return 0;
}

/*
 * 设置固件库版本号
 * */
int HAL_PARAMS_Set_System_fwlib_ver(const char* buffer) {
    uint16_t templen;

    if (buffer!=NULL) {
        templen = MIN(sizeof(intorobot_system_params.fwlib_ver)-1, strlen(buffer));
        memset(intorobot_system_params.fwlib_ver, 0, sizeof(intorobot_system_params.fwlib_ver));
        memcpy(intorobot_system_params.fwlib_ver, buffer, templen);
        return 0;
    }
    return -1;
}

/*
 * 读取参数配置标志
 * */
CONFIG_FLAG_TypeDef HAL_PARAMS_Get_System_config_flag(void) {
   return (CONFIG_FLAG_TypeDef)intorobot_system_params.config_flag;
}

/*
 * 保存参数配置标志
 * */
int HAL_PARAMS_Set_System_config_flag(CONFIG_FLAG_TypeDef flag) {
    intorobot_system_params.config_flag = flag;
    return 0;
}

