#if 0
// pull in the sources from the HAL. It's a bit of a hack, but is simpler than trying to link the
// full hal library.
#include "../src/neutron/params_impl.h"

#else

#ifndef PARAMS_IMPL_H_
#define PARAMS_IMPL_H_

#include <stdint.h>
#include <stdio.h>

#define  BOOT_PARAMS_HEADER    0x5aa5f66f    // bootloader参数区
#define  SYSTEM_PARAMS_HEADER  0x5aa5f66f    // 系统参数区


//参数区大小不能变动
typedef struct __attribute__((packed))
{
    uint32_t header;          // 系统标志参数区标志  固定为0x5AA5F66F
    uint32_t boot_version;    // bootloader版本号
    uint8_t  boot_flag;       // bootloader处理标志 0:正常启动  1:进入默认程序恢复  2:进入esp8266串口转发  3:恢复出厂  4:进入在线应用升级
    uint8_t  initparam_flag;  // 参数初始化标志  0:应用参数不初始化  1:应用参数恢复出厂设置(保留密钥)  2:应用参数恢复出厂设置(不保留密钥)
    uint8_t  reserved[61];    // 参数预留区 每添加一个参数，预留区大小减1
    uint8_t  end;    // 参数预留区 每添加一个参数，预留区大小减1
}boot_params_t;


typedef struct __attribute__((packed))
{
    uint32_t header;          // 系统标志参数区标志  固定为0x5AA5F66F
    char     fwlib_ver[24];        // 固件库版本号
    char     subsys_ver[24];       // 子系统模块版本号
    uint8_t  reserverd[96];        // 版本号预留区 每添加一个版本号，预留区大小减1
    uint8_t  config_flag;          // 是否进入配置模式  0:不进入  1:进入
    uint8_t  ota_flag;             // 是否上报在线升级成功状态   0:不上报  1:上报
    uint8_t  reset_flag;           // 是否上报重启成功状态   0:不上报  1:上报
    uint8_t  at_mode;              // 是否已经灌装密钥  0:未灌装 1:已经灌装
    uint8_t  sv_select;            // 是否选择默认服务参数  0:使用 1:不使用
    uint8_t  reserverd1[31];       // 版本号预留区 每添加一个版本号，预留区大小减1
    char     device_id[52];        // 设备序列号
    char     access_token[52];     // 设备access_token
    char     sv_domain[52];        // 服务器域名
    int      sv_port;              // 服务器端口
    char     dw_domain[52];        // 服务器下载域名
    float    zone;                 // 核心板所在时区。用于实时时钟。

    uint8_t  reserved2[599];       // 参数预留区 每添加一个参数，预留区大小减1
    uint8_t  end;
} system_params_t;

#endif /*SYSTEM_PARAMS_H_*/

#endif
