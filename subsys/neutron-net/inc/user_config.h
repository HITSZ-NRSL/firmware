#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#include "eboot_command.h"
#include "at_ex_command.h"
#include "upgrade.h"

/*版本号说明  前三位 1.0.0 esp8266版本号 第4位 为stm32 bootloader版本号，该位不要轻易变动*/
//#define  WIFIVERSION               "1.0.2.1"
/* 修改点
 * 1.esp8266 sdk库升级至v2.0版本
 */
#define  WIFIVERSION               "2.0.0.1"

/*
 * 1.stm32 bootloader升级至v2. 解决恢复程序和烧录程序失败的情况
 */
//#define  WIFIVERSION               "2.0.0.2"

//#define DEBUG_BUILD

#ifdef DEBUG_BUILD
#define DEBUG os_printf
#else
#define DEBUG
#endif

extern struct eboot_command cmd_info;

#endif
