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
    uint8_t  at_mode;
    uint8_t  device_id[52]={0}, access_token[52]={0}, activation_code[52]={0};

    switch(psystem_params->at_mode)
    {
        case 1:      //Activation By Personalization  //已经灌好密钥
            at_mode = psystem_params->at_mode;
            memcpy(device_id, psystem_params->device_id, sizeof(psystem_params->device_id));
            memcpy(access_token, psystem_params->access_token, sizeof(psystem_params->access_token));
            init_system_params(psystem_params);
            psystem_params->at_mode = at_mode;
            memcpy(psystem_params->device_id, device_id, sizeof(psystem_params->device_id));
            memcpy(psystem_params->access_token, access_token, sizeof(psystem_params->access_token));
            break;
        case 2:      //Over-The-Air Activation //灌装激活码  未激活
        case 3:      //灌装激活码 已激活
            memcpy(device_id, psystem_params->device_id, sizeof(psystem_params->device_id));
            memcpy(activation_code, psystem_params->activation_code, sizeof(psystem_params->activation_code));
            init_system_params(psystem_params);
            psystem_params->at_mode = 2; //灌装激活码  未激活
            memcpy(psystem_params->device_id, device_id, sizeof(psystem_params->device_id));
            memcpy(psystem_params->activation_code, activation_code, sizeof(psystem_params->activation_code));
            break;
        default:     //没有密钥信息
            init_system_params(psystem_params);
            break;
    }
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
/********************************************************************************
 *  添加参数
 ********************************************************************************/

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

/*
 * 保存设备ID
 * */
uint16_t HAL_PARAMS_Get_System_device_id(char* buffer, uint16_t len) {
    uint16_t templen;

    if (buffer!=NULL && len>0) {
        templen = MIN(strlen(intorobot_system_params.device_id), len-1);
        memset(buffer, 0, len);
        memcpy(buffer, intorobot_system_params.device_id, templen);
        return templen;
    }
    return 0;
}

/*
 * 设置设备ID
 * */
int HAL_PARAMS_Set_System_device_id(const char* buffer) {
    uint16_t templen;

    if (buffer!=NULL) {
        templen = MIN(sizeof(intorobot_system_params.device_id)-1, strlen(buffer));
        memset(intorobot_system_params.device_id, 0, sizeof(intorobot_system_params.device_id));
        memcpy(intorobot_system_params.device_id, buffer, templen);
        return 0;
    }
    return -1;
}

/*
 * 读取设备access token
 * */
uint16_t HAL_PARAMS_Get_System_access_token(char* buffer, uint16_t len) {
    uint16_t templen;

    if (buffer!=NULL && len>0) {
        templen = MIN(strlen(intorobot_system_params.access_token), len-1);
        memset(buffer, 0, len);
        memcpy(buffer, intorobot_system_params.access_token, templen);
        return templen;
    }
    return 0;
}

/*
 * 设置设备access token
 * */
int HAL_PARAMS_Set_System_access_token(const char* buffer) {
    uint16_t templen;

    if (buffer!=NULL) {
        templen = MIN(sizeof(intorobot_system_params.access_token)-1, strlen(buffer));
        memset(intorobot_system_params.access_token, 0, sizeof(intorobot_system_params.access_token));
        memcpy(intorobot_system_params.access_token, buffer, templen);
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
 * 读取服务器域名
 * */
uint16_t HAL_PARAMS_Get_System_sv_domain(char* buffer, uint16_t len) {
    uint16_t templen;

    if (buffer!=NULL && len>0) {
        templen = MIN(strlen(intorobot_system_params.sv_domain), len-1);
        memset(buffer, 0, len);
        memcpy(buffer, intorobot_system_params.sv_domain, templen);
        return templen;
    }
    return 0;
}

/*
 * 设置服务器域名
 * */
int HAL_PARAMS_Set_System_sv_domain(const char* buffer) {
    uint16_t templen;

    if (buffer!=NULL) {
        templen = MIN(sizeof(intorobot_system_params.sv_domain)-1, strlen(buffer));
        memset(intorobot_system_params.sv_domain, 0, sizeof(intorobot_system_params.sv_domain));
        memcpy(intorobot_system_params.sv_domain, buffer, templen);
        return 0;
    }
    return -1;
}

/*
 * 读取服务器端口号
 * */
int HAL_PARAMS_Get_System_sv_port(void) {
    return intorobot_system_params.sv_port;
}

/*
 * 设置服务器端口号
 * */
int HAL_PARAMS_Set_System_sv_port(int port) {
    intorobot_system_params.sv_port = port;
    return 0;
}

/*
 * 读取文件下载域名
 * */
uint16_t HAL_PARAMS_Get_System_dw_domain(char* buffer, uint16_t len) {
    uint16_t templen;

    if (buffer!=NULL && len>0) {
        templen = MIN(strlen(intorobot_system_params.dw_domain), len-1);
        memset(buffer, 0, len);
        memcpy(buffer, intorobot_system_params.dw_domain, templen);
        return templen;
    }
    return 0;
}

/*
 * 设置文件下载域名
 * */
int HAL_PARAMS_Set_System_dw_domain(const char* buffer) {
    uint16_t templen;

    if (buffer!=NULL) {
        templen = MIN(sizeof(intorobot_system_params.dw_domain)-1, strlen(buffer));
        memset(intorobot_system_params.dw_domain, 0, sizeof(intorobot_system_params.dw_domain));
        memcpy(intorobot_system_params.dw_domain, buffer, templen);
        return 0;
    }
    return -1;
}

/*
 * 读取子系统升级标志
 * */
uint8_t HAL_PARAMS_Get_System_subsys_flag(void) {
    return intorobot_system_params.subsys_flag;
}

/*
 * 保存子系统升级标志
 * */
int HAL_PARAMS_Set_System_subsys_flag(uint8_t flag) {
    intorobot_system_params.subsys_flag = flag;
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
 * 读取子系统版本号
 * */
uint16_t HAL_PARAMS_Get_System_subsys_ver(char* buffer, uint16_t len) {
    uint16_t templen;

    if (buffer!=NULL && len>0) {
        templen = MIN(strlen(intorobot_system_params.subsys_ver), len-1);
        memset(buffer, 0, len);
        memcpy(buffer, intorobot_system_params.subsys_ver, templen);
        return templen;
    }
    return 0;
}

/*
 * 设置文件下载域名
 * */
int HAL_PARAMS_Set_System_subsys_ver(const char* buffer) {
    uint16_t templen;

    if (buffer!=NULL) {
        templen = MIN(sizeof(intorobot_system_params.subsys_ver)-1, strlen(buffer));
        memset(intorobot_system_params.subsys_ver, 0, sizeof(intorobot_system_params.subsys_ver));
        memcpy(intorobot_system_params.subsys_ver, buffer, templen);
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

/*添加激活码*/
/*
 * 读取设备activation_code
 * */
uint16_t HAL_PARAMS_Get_System_activation_code(char* buffer, uint16_t len) {
    uint16_t templen;

    if (buffer!=NULL && len>0) {
        templen = MIN(strlen(intorobot_system_params.activation_code), len-1);
        memset(buffer, 0, len);
        memcpy(buffer, intorobot_system_params.activation_code, templen);
        return templen;
    }
    return 0;
}

/*
 * 设置设备activation_code
 * */
int HAL_PARAMS_Set_System_activation_code(const char* buffer) {
    uint16_t templen;

    if (buffer!=NULL) {
        templen = MIN(sizeof(intorobot_system_params.activation_code)-1, strlen(buffer));
        memset(intorobot_system_params.activation_code, 0, sizeof(intorobot_system_params.activation_code));
        memcpy(intorobot_system_params.activation_code, buffer, templen);
        return 0;
    }
    return -1;
}

/*
 * 读取http服务器域名
 * */
uint16_t HAL_PARAMS_Get_System_http_domain(char* buffer, uint16_t len) {
    uint16_t templen;

    if (buffer!=NULL && len>0) {
        templen = MIN(strlen(intorobot_system_params.http_domain), len-1);
        memset(buffer, 0, len);
        memcpy(buffer, intorobot_system_params.http_domain, templen);
        return templen;
    }
    return 0;
}

/*
 * 设置http服务器域名
 * */
int HAL_PARAMS_Set_System_http_domain(const char* buffer) {
    uint16_t templen;

    if (buffer!=NULL) {
        templen = MIN(sizeof(intorobot_system_params.http_domain)-1, strlen(buffer));
        memset(intorobot_system_params.http_domain, 0, sizeof(intorobot_system_params.http_domain));
        memcpy(intorobot_system_params.http_domain, buffer, templen);
        return 0;
    }
    return -1;
}

/*
 * 读取http服务器端口号
 * */
int HAL_PARAMS_Get_System_http_port(void) {
    return intorobot_system_params.http_port;
}

/*
 * 设置http服务器端口号
 * */
int HAL_PARAMS_Set_System_http_port(int port) {
    intorobot_system_params.http_port = port;
    return 0;
}

