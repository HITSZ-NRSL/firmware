#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#include "eboot_command.h"
#include "at_ex_command.h"
#include "upgrade.h"

/*版本号说明  前三位 1.0.0 esp8266版本号 第4位 为stm32 bootloader版本号，该位不要轻易变动*/
//#define  WIFIVERSION               "1.0.2.1"
//#define  WIFIVERSION               "2.0.0.1" //1.esp8266 sdk库升级至v2.0版本
#define  WIFIVERSION               "2.0.0.2"   //bootloader 版本更新至v2  1.修正flash烧写失败处理,保证烧写成功  2. esp8266升级模式，波特率可以通过usb设置


//#define DEBUG_BUILD

#ifdef DEBUG_BUILD
#define DEBUG os_printf
#else
#define DEBUG
#endif

extern struct eboot_command cmd_info;

#endif
