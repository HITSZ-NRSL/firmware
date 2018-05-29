#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#include "eboot_command.h"
#include "at_ex_command.h"
#include "upgrade.h"

//#define DEBUG_BUILD

#ifdef DEBUG_BUILD
#define DEBUG os_printf
#else
#define DEBUG
#endif

extern struct eboot_command cmd_info;

#define SUBSYS_VERSION_STRING 2.0.3.3

#if 0
# 模组系统版本号

#neutorn 子系统版本号放在esp8266上，stm32通过At指令获取子系统版本号
#子系统的版本号，主要用于管理esp8266程序、默认应用程序、stm32的bootloader程序。
#版本号组成:  总共4位，其中第四位:为stm32 bootloader版本号，必须与升级包中的bootloader版本一致

##############################################
#版本号：1.0.2.1
#修改者：
#修改点：
#1. 初始化版本

##############################################
#版本号：2.0.0.1
#修改者：
#修改点：
#1. esp8266 sdk库升级至v2.0版本

##############################################
#版本号：2.0.0.2
#修改者：
#修改点：
#1. bootloader 版本更新至v2
#2. 修正flash烧写失败处理,保证烧写成功
#3. esp8266升级模式，波特率可以通过usb设置

##############################################
#版本号：2.0.1.3
#修改者：
#修改点：
# 1. 默认程序更新 * 添加激活流程  *简化配置模式
# 2. bootloade3 版本更新至v3  *.简化工作模式 *.简化灯的变化,开始统一为白灯

##############################################
#版本号：2.0.2.3
#修改者：
#修改点：
# 1. MQTT采取加密方式

##############################################
#版本号：2.0.3.3
#修改者：
#修改点：
# 1. 默认程序归属至默认产品

#endif

#endif
