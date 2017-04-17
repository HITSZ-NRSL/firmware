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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef PARAMS_HAL_H_
#define PARAMS_HAL_H_

#ifdef __cplusplus
extern "C" {
#endif


/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef enum
{
    BOOT_FLAG_NORMAL            = 0,
    BOOT_FLAG_DEFAULT_RESTORE   = 1,
    BOOT_FLAG_SERIAL_COM        = 2,
    BOOT_FLAG_FACTORY_RESET     = 3,
    BOOT_FLAG_OTA_UPDATE        = 4,
    BOOT_FLAG_ALL_RESET         = 5,
    BOOT_FLAG_USB_DFU           = 6
} BOOT_FLAG_TypeDef;

typedef enum
{
    INITPARAM_FLAG_NORMAL            = 0,
    INITPARAM_FLAG_FACTORY_RESET     = 1,
    INITPARAM_FLAG_ALL_RESET         = 2
} INITPARAM_FLAG_TypeDef;

typedef enum
{
    AT_MODE_FLAG_NONE            = 0, //没有密钥信息
    AT_MODE_FLAG_ABP             = 1, //Activation By Personalization  //已经灌好密钥
    AT_MODE_FLAG_OTAA_INACTIVE   = 2, //Over-The-Air Activation //灌装激活码  未激活
    AT_MODE_FLAG_OTAA_ACTIVE     = 3  //灌装激活码 已激活
} AT_MODE_FLAG_TypeDef;

typedef enum
{
    SV_SELECT_FLAG_DEFAULT       = 0, //使用默认参数
    SV_SELECT_FLAG_CUSTOM        = 1 //使用自定义参数
} SV_SELECT_FLAG_TypeDef;

/* Exported constants --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

void HAL_PARAMS_Init_All_System_Params(void);
void HAL_PARAMS_Init_Fac_System_Params(void);
void HAL_PARAMS_Init_Boot_Params(void);
void HAL_PARAMS_Load_System_Params(void);
void HAL_PARAMS_Load_Boot_Params(void);
void HAL_PARAMS_Save_Params(void);

uint32_t HAL_PARAMS_Get_Boot_boot_version(void);
int HAL_PARAMS_Set_Boot_boot_version(uint32_t version);
BOOT_FLAG_TypeDef HAL_PARAMS_Get_Boot_boot_flag(void);
int HAL_PARAMS_Set_Boot_boot_flag(BOOT_FLAG_TypeDef flag);
INITPARAM_FLAG_TypeDef HAL_PARAMS_Get_Boot_initparam_flag(void);
int HAL_PARAMS_Set_Boot_initparam_flag(INITPARAM_FLAG_TypeDef flag);

uint32_t HAL_PARAMS_Get_Boot_ota_app_size(void);
int HAL_PARAMS_Set_Boot_ota_app_size(uint32_t size);
uint32_t HAL_PARAMS_Get_Boot_def_app_size(void);
int HAL_PARAMS_Set_Boot_def_app_size(uint32_t size);
uint32_t HAL_PARAMS_Get_Boot_boot_size(void);
int HAL_PARAMS_Set_Boot_boot_size(uint32_t size);
#ifdef __cplusplus
}
#endif

#endif
