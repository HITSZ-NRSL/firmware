## v1.7.0

### 新增功能
- 支持IntoYun固件升级(#184)
- 添加HTTP文件下载接口
- LoRaWAN节点新增62.5k带宽支持

### 功能优化
- LoRaWAN节点去掉Info信息上送，设备采取AppEUI进行产品绑定

### 漏洞修复
- 空

### 内部调整
- tools自动代码脚本采用python编写


## v1.6.10

### 新增功能
- lorawan增加电池电压接口

### 功能优化
- 修改esp8266 flash操作保护
- l6默认打开usb串口
- 修改lorawan rtc和sleep接口

### 漏洞修复
- 修正esp8266网页串口配置网络重启问题(#177)
- 修正gprs模组配置getinfo信息未显示全
- 修正fig 子系统版本号读取错误
- 修正neutron子系统版本号读取错误
- 修正esp8266使用舵机时，获取ssid或者配置网络时重启问题
- 修正ant eeprom put接口错误

### 内部调整
- 规范core_hal接口。
- 修正按配置按键退出执行配置初始化bug。优化配置初始化和配置完成执行代码
- lorawan去掉rtc计算唤醒时间等接口


## v1.6.9

### 新增功能
- [[PR #166]](https://github.com/HITSZ-NRSL/firmware/pull/166) esp32 添加tcp server功能。
- [[PR #166]](https://github.com/HITSZ-NRSL/firmware/pull/166) esp8266 添加tcp server功能。

### 功能优化
- [[PR #162]](https://github.com/HITSZ-NRSL/firmware/pull/162) lorawan将发射功率修改以3的倍率递减，修改后的发射功率为20/17/14/11/7/4db。
- [[PR #166]](https://github.com/HITSZ-NRSL/firmware/pull/166) esp8266 SDK 与 esp8266 arduino 同步。对应master commit:074402e23b72305af2fa23c9944c7b96058f06b0。
- [[PR #166]](https://github.com/HITSZ-NRSL/firmware/pull/166) 去掉wiring_tcpclient.h tcp buffer宏定义判断。对wifi模组统一接口。
- [[PR #166]](https://github.com/HITSZ-NRSL/firmware/pull/166) tcp_server write接口修改成往所有连接server client发送数据。
- [[PR #169]](https://github.com/HITSZ-NRSL/firmware/pull/169) 去掉esp8266 sdk log信息。

### 漏洞修复
- [[PR #159]](https://github.com/HITSZ-NRSL/firmware/pull/159) 修正nut 和 w323 链接文件。否则编译大小提示大小出错。
- [[PR #159]](https://github.com/HITSZ-NRSL/firmware/pull/159) 修正透传型数据点读取错误。
- [[PR #159]](https://github.com/HITSZ-NRSL/firmware/pull/159) 修正用户设置非115200波特率，设备无法配置问题。
- [[PR #159]](https://github.com/HITSZ-NRSL/firmware/pull/159) 修正对应wifi设备，手机和web打开页面，无法获取数据。
- [[PR #162]](https://github.com/HITSZ-NRSL/firmware/pull/162) 修正lorawan获取发射功率错误问题。
- [[PR #163]](https://github.com/HITSZ-NRSL/firmware/pull/163) 修正fig和w323 调用舵机接口无法编译问题。
- [[PR #165]](https://github.com/HITSZ-NRSL/firmware/pull/165) 修正while(!WiFi.ready()) 后直接调用WiFi.localIP()无法获取ip地址。
- [[PR #165]](https://github.com/HITSZ-NRSL/firmware/pull/165) 修正SYSTEM_MODE(MANUAL);无法连接WIFI问题。
- [[PR #169]](https://github.com/HITSZ-NRSL/firmware/pull/169) 修正esp8266 调用while(!Serial.available()); 出现看门狗超时。
- [[PR #169]](https://github.com/HITSZ-NRSL/firmware/pull/169) 修正neutron配置网络失败问题, 由于AT指令冲突。。

### 内部调整
- [[PR #166]](https://github.com/HITSZ-NRSL/firmware/pull/166) 调整esp32 socket处理方式，达到统一接口目的。


## v1.6.8

### 新增功能
- 无

### 功能优化
- [[PR #152]](https://github.com/HITSZ-NRSL/firmware/pull/152) 更新esp32 SDK
- [[PR #153]](https://github.com/HITSZ-NRSL/firmware/pull/153) 更新esp8266 SDK
- [[PR #154]](https://github.com/HITSZ-NRSL/firmware/pull/154) 修改esp8266 链接文件，保持与intoyuniot一致

### 漏洞修复
- 无

### 内部调整
- 无


## v1.6.7

### 新增功能
- [[PR #142]](https://github.com/HITSZ-NRSL/firmware/pull/142) 添加部分头文件，兼容arduino库
- [[PR #142]](https://github.com/HITSZ-NRSL/firmware/pull/142) 向用户提供mqtt接口和json接口
- [[PR #146]](https://github.com/HITSZ-NRSL/firmware/pull/146) LoRaWan移植官方v4.4.0版本
- [[PR #146]](https://github.com/HITSZ-NRSL/firmware/pull/146) LoRaWan默认3个通道的频率和速率都可以修改
- [[PR #146]](https://github.com/HITSZ-NRSL/firmware/pull/146) LoRa频率设置范围修改为137-515MHz
- [[PR #146]](https://github.com/HITSZ-NRSL/firmware/pull/146) LoRa休眠时去掉内部rtc计数，调用休眠接口直接进入休眠
- [[PR #146]](https://github.com/HITSZ-NRSL/firmware/pull/146) stm32l1xx增加IO快速读写接口适用公共库

### 功能优化
- [[PR #142]](https://github.com/HITSZ-NRSL/firmware/pull/142) gl2000/gl2100 bootloader添加硬件自检程序

### 漏洞修复
- [[PR #142]](https://github.com/HITSZ-NRSL/firmware/pull/142) gl2000、gl2100 pwm_hal.c完善hal层接口，防止编译出错。

### 内部调整
- 无


## v1.6.6

### 新增功能
- [[PR #132]](https://github.com/HITSZ-NRSL/firmware/pull/132) 添加System.onSysTick()函数

### 功能优化
- [[PR #132]](https://github.com/HITSZ-NRSL/firmware/pull/132) 去掉mqtt v1版本topic，支持在线烧录显示进度
- [[PR #133]](https://github.com/HITSZ-NRSL/firmware/pull/133) 模组 配置状态默认不保存，上电退出配置模式
- [[PR #135]](https://github.com/HITSZ-NRSL/firmware/pull/135) 添加RXD TXD宏定义
- [[PR #136]](https://github.com/HITSZ-NRSL/firmware/pull/136) 兼容arduino

### 漏洞修复
- 无

### 内部调整
- 无


## v1.6.5

### 新增功能
- [[PR #128]](https://github.com/HITSZ-NRSL/firmware/pull/128) Cloud 添加keepAlive接口

### 功能优化
- [[PR #126]](https://github.com/HITSZ-NRSL/firmware/pull/126) 去除激活流程
- [[PR #127]](https://github.com/HITSZ-NRSL/firmware/pull/127) 获取时间戳采取http方式
- [[PR #127]](https://github.com/HITSZ-NRSL/firmware/pull/127) Cloud.connect() 不采取死等方式。否则从模式连接网络时，将堵死，无法接受AT指令。
- [[PR #128]](https://github.com/HITSZ-NRSL/firmware/pull/128) mqtt设备未注册，默认不连接平台
- [[PR #128]](https://github.com/HITSZ-NRSL/firmware/pull/128) mqtt 数据包大小从512 修改成 768

### 漏洞修复
- [[PR #127]](https://github.com/HITSZ-NRSL/firmware/pull/127) usb_hal发送数据添加超时处理。修正USB未打开串口，usb发送数据卡死的问题。
- [[PR #127]](https://github.com/HITSZ-NRSL/firmware/pull/127) 修正w323串口无法收发问题。
- [[PR #127]](https://github.com/HITSZ-NRSL/firmware/pull/127) esp32 添加 dac功能

### 内部调整
- [[PR #126]](https://github.com/HITSZ-NRSL/firmware/pull/126) 优化subsys_version处理
- [[PR #126]](https://github.com/HITSZ-NRSL/firmware/pull/126) 优化默认网络参数处理
- [[PR #128]](https://github.com/HITSZ-NRSL/firmware/pull/128) 调整neutron子系统版本号


## v1.6.4

### 新增功能
- [[PR #113]](https://github.com/HITSZ-NRSL/firmware/pull/113) 添加GL2000和GL2100设备类型
- [[PR #114]](https://github.com/HITSZ-NRSL/firmware/pull/114) mqtt接入添加加密方式。去掉activation_code。

### 功能优化
- [[PR #115]](https://github.com/HITSZ-NRSL/firmware/pull/115) 数据点定义去掉发送策略，简化用户接口。

### 漏洞修复
- 无

### 内部调整
- 无


## v1.6.3-rc.1

### 新增功能
- 无

### 功能优化
- [[PR #99]](https://github.com/HITSZ-NRSL/firmware/pull/99) [ant/l6] 固定扩频因子修改为9。
- [[PR #101]](https://github.com/HITSZ-NRSL/firmware/pull/101) [ant/l6] 精简ant和l6大小。

### 漏洞修复
- [[PR #98]](https://github.com/HITSZ-NRSL/firmware/pull/98) [all] 编译处理的framework在macos上loragateway项目无法启动。原因:boot/system参数结构体__attribute__((packed))参数。因为参数全局变量地址可能不是4字节对齐。导致flash read操作死机。
- [[PR #99]](https://github.com/HITSZ-NRSL/firmware/pull/99) [ant/l6] lorawan主从模式增加休眠接口。
- [[PR #99]](https://github.com/HITSZ-NRSL/firmware/pull/99) [ant/l6] 解决激活失败后不能重新入网bug。
- [[PR #100]](https://github.com/HITSZ-NRSL/firmware/pull/100) [ant/l6] lorawan休眠唤醒后串口无法接收数据，原因是在Serial.end()将申请的数据缓冲区释放了。

### 内部调整
- 无


## v1.6.2-rc.1

### 新增功能
- 无

### 功能优化
- [[PR #93]](https://github.com/HITSZ-NRSL/firmware/pull/93) [esp8266/esp32] 添加获取堆空闲大小接口
- [[PR #94]](https://github.com/HITSZ-NRSL/firmware/pull/94) [w67/w323] 更新w67/w323 flash地址分布，去掉bootloaer缓冲区和默认程序区。合并到文件系统区。

### 漏洞修复
- [[PR #93]](https://github.com/HITSZ-NRSL/firmware/pull/93) [w67/neutron/fox] 修正软件定时器设置定时周期无效问题

### 内部调整
- 无


## v1.6.1-rc.1

### 新增功能
- 无

### 功能优化
- [[PR #90]](https://github.com/HITSZ-NRSL/firmware/pull/90) [all] arduino.h 修改成 Arduino.h 兼容arduino
- [[PR #91]](https://github.com/HITSZ-NRSL/firmware/pull/91) [ant/l6] ant/l6 配置模式支持串口和USB口
- [[PR #91]](https://github.com/HITSZ-NRSL/firmware/pull/91) [ant/l6] 实现空闲堆大小统计接口

### 漏洞修复
- [[PR #91]](https://github.com/HITSZ-NRSL/firmware/pull/91) [ant/l6] 修正ant l6配置模式,USB驱动出现黄色感叹号的问题。该问题是由于配置模式时每次上电都保存配置模式，导致中断关闭，引起USB驱动问题

### 内部调整
- [[PR #90]](https://github.com/HITSZ-NRSL/firmware/pull/90) [all] 添加intoyuniot frameworks生成脚本


## v1.6.0-rc.1

### 新增功能
- [[PR #79]](https://github.com/HITSZ-NRSL/firmware/pull/79) [fox/ant/l6] 新增fox、ant、l6的支持
- [[PR #86]](https://github.com/HITSZ-NRSL/firmware/pull/86) [all] 新增System.set() System.get()接口设置系统参数，涉及board_id、board_name等
- [[PR #86]](https://github.com/HITSZ-NRSL/firmware/pull/86) [all] 新增System.enabeFeature()等接口，涉及设备信息上送、ota升级等.
- [[PR #86]](https://github.com/HITSZ-NRSL/firmware/pull/86) [neutron/nut/w67/w323] 添加软件定时器接口.

### 功能优化
- 无

### 漏洞修复
- 无

### 内部调整
- 无


## v1.5.4-rc.2

### 新增功能
- 无

### 功能优化
- [[PR #75]](https://github.com/HITSZ-NRSL/firmware/pull/75) [nut/fig/w67/w323] esp8266 esp32 添加u8等定义
- [[PR #75]](https://github.com/HITSZ-NRSL/firmware/pull/75) [nut/w67] esp8266移植sdk2.0后添加支持servo功能

### 漏洞修复
- [[PR #75]](https://github.com/HITSZ-NRSL/firmware/pull/75) [wifi/gprs] 修正IntoRobot.Publish()发送错误。版本号当做了字符串长度，导致每次只发一个数据
- [[PR #75]](https://github.com/HITSZ-NRSL/firmware/pull/75) [nut/w67] 修正esp8266 AD读值bug 之前乘以2倍

### 内部调整
- 无


## v1.5.4-rc.1

### 新增功能
- 无

### 功能优化
- [[PR #63]](https://github.com/HITSZ-NRSL/firmware/pull/63) [neutron/nut/fig/ant/fox] 简化设备bootloader和app设备配置模式
- [[PR #63]](https://github.com/HITSZ-NRSL/firmware/pull/63) [neutron/nut/fig/ant/fox] 简化设备bootloader颜色变化
- [[PR #63]](https://github.com/HITSZ-NRSL/firmware/pull/63) [neutron/nut/fig/w67/w323] 添加imlink失败重新进入imlink配置功能
- [[PR #65]](https://github.com/HITSZ-NRSL/firmware/pull/65) [nut/w67] esp8266 sdk更新至2.0
- [[PR #67]](https://github.com/HITSZ-NRSL/firmware/pull/67) [neutron] 去掉neutron modem AT指令解析函数延时，加快平台控制响应
- [[PR #67]](https://github.com/HITSZ-NRSL/firmware/pull/67) [neutron] 优化neutron wlan_setup流程，区别设备进入配置模式再手动退出配置模式，能够正常联网.

### 漏洞修复
- [[PR #63]](https://github.com/HITSZ-NRSL/firmware/pull/62) [neutron/nut/fig/w67/w323] 修正tcp stop没有清除缓存，导致连接状态没有变化
- [[PR #65]](https://github.com/HITSZ-NRSL/firmware/pull/65) [nut/w67] 修正串口下载后应用运行软复位无法重启的问题。解决方法:下载后硬件复位模块。

### 内部调整
- [[PR #62]](https://github.com/HITSZ-NRSL/firmware/pull/62) [all] 添加独立工程编译
- [[PR #62]](https://github.com/HITSZ-NRSL/firmware/pull/62) [all] 修改编译目标文件名称
- [[PR #63]](https://github.com/HITSZ-NRSL/firmware/pull/63) [all] 子系统版本版本统一放入build/subsys_version.mk统一管理
- [[PR #67]](https://github.com/HITSZ-NRSL/firmware/pull/67) [all] dfu-util upload-reset放入tools文件夹 修正dfu-util下载成功提示失败问题
- [[PR #68]](https://github.com/HITSZ-NRSL/firmware/pull/68) [L6/Ant] 变更LoRaWan驱动库


## v1.5.3-rc.2

### 新增功能
- 无

### 功能优化
- [[PR #60]](https://github.com/HITSZ-NRSL/firmware/pull/60) [all] 字符串和透传型数据点定义添加最大长度
- [[PR #60]](https://github.com/HITSZ-NRSL/firmware/pull/60) [all] 去除数据点功能开关

### 漏洞修复
- [[PR #59]](https://github.com/HITSZ-NRSL/firmware/pull/59) [fig] 修复fig第一次上电配置完成后无法转白灯问题
- [[PR #61]](https://github.com/HITSZ-NRSL/firmware/pull/61) [all] 添加默认数据点处理。包括复位和获取全部可上送数据点

### 内部调整
- 无


## v1.5.3-rc.1

### 新增功能
- [[PR #58]](https://github.com/HITSZ-NRSL/firmware/pull/58) [all] 数据点组织格式统一使用二进制方式
- [[PR #58]](https://github.com/HITSZ-NRSL/firmware/pull/58) [all] 添加事件通知接口
- [[PR #58]](https://github.com/HITSZ-NRSL/firmware/pull/58) [all] 数据点发送添加系统自动处理方式

### 功能优化
- [[PR #58]](https://github.com/HITSZ-NRSL/firmware/pull/58) [nut/w6/w7] esp8266 process 放入到system层实现

### 漏洞修复
- [[PR #58]](https://github.com/HITSZ-NRSL/firmware/pull/58) [wifi] 修正恢复出厂设置后，设备无法注册和激活, http地址错误
- [[PR #58]](https://github.com/HITSZ-NRSL/firmware/pull/58) [nut/w6/w7] 修正nut下载软件包后，通过串口配置wifi无法成功

### 内部调整
- 无


## v1.5.2-rc.1

### 新增功能
- [[PR #51]](https://github.com/HITSZ-NRSL/firmware/pull/51) [ant] 添加ant的支持
- [[PR #52]](https://github.com/HITSZ-NRSL/firmware/pull/52) [ant] fig支持pwm功能
- [[PR #54]](https://github.com/HITSZ-NRSL/firmware/pull/54) [fox] 添加fox支持,包括bootloader和应用程序

### 功能优化
- [[PR #51]](https://github.com/HITSZ-NRSL/firmware/pull/51) [all] 优化IntoRobot 数据点处理接口
- [[PR #53]](https://github.com/HITSZ-NRSL/firmware/pull/53) [all] IntoRobot 数据点接口从DataPoint改成Datapoint
- [[PR #54]](https://github.com/HITSZ-NRSL/firmware/pull/54) [neutron/nut/fig/w67/w323] 获取网络状态采取通知机制

### 漏洞修复
- [[PR #54]](https://github.com/HITSZ-NRSL/firmware/pull/55) [all] STM32L151xB 芯片 ajson print申请变量大小改成512个字节
- [[PR #57]](https://github.com/HITSZ-NRSL/firmware/pull/56) [all] 修正调用IntoRobot.printf()函数编译出错，原因在于基类的继承属性

### 内部调整
- [[PR #54]](https://github.com/HITSZ-NRSL/firmware/pull/54) [all] PRODUCT_ID 默认值修改成空, PRODUCT_SECRET 默认值修改成空, PRODUCT_VERSION 默认值修改成1
- [[PR #57]](https://github.com/HITSZ-NRSL/firmware/pull/55) [all] intorobot上送版本信息添加board字段


## v1.5.1-rc.1

### 新增功能
- [[PR #45]](https://github.com/HITSZ-NRSL/firmware/pull/45) [w67/w323] 发布w67/w322
- [[PR #48]](https://github.com/HITSZ-NRSL/firmware/pull/48) [fig] fig bootloader添加模式处理
- [[PR #49]](https://github.com/HITSZ-NRSL/firmware/pull/49) [fig] fig 支持SPI I2C AD功能

### 功能优化
- 无

### 漏洞修复
- [[PR #45]](https://github.com/HITSZ-NRSL/firmware/pull/45) [fig] 修正fig管教映射
- [[PR #47]](https://github.com/HITSZ-NRSL/firmware/pull/47) [neutron] 修正neutron 运行过程中死机问题, 该问题与freeRtos中断优先级设置有关系.
- [[PR #50]](https://github.com/HITSZ-NRSL/firmware/pull/50) [neutron] neutron bootloader 版本升级至v2版本 修正esp8266在不同操作系统升级失败问题。通过USB选择波特率来置esp8266串口通讯波特率。

### 内部调整
- [[PR #45]](https://github.com/HITSZ-NRSL/firmware/pull/45) [all] i2c i2s spi usartserial usbserial 为了防止其他类构造函数调用，统一采取函数方式实现
- [[PR #46]](https://github.com/HITSZ-NRSL/firmware/pull/46) [fig] fig AD功能调试OK，引脚读写功能OK
- [[PR #50]](https://github.com/HITSZ-NRSL/firmware/pull/50) [neutron] 更新neutron子系统版本至2.0.0.2
- [[PR #50]](https://github.com/HITSZ-NRSL/firmware/pull/50) [neutron] 修改neutron的USB中断优先级为2，防止插拔USB阻止esp8266串口接收数据


## v1.5.0-rc.2

### 新增功能
- 无

### 功能优化
- [[PR #41]](https://github.com/HITSZ-NRSL/firmware/pull/41) [neutron] neutron添加单任务和多任务开关。默认单任务。

### 漏洞修复
- [[PR #41]](https://github.com/HITSZ-NRSL/firmware/pull/41) [neutron] 因为neutron需要实时接收esp8266数据，所有不允许关闭总中断。ext_hal.c 里面的开启和关闭中断，只能处理用户中断.
- [[PR #42]](https://github.com/HITSZ-NRSL/firmware/pull/42) [nut] 修改nut jsonGetValue()重启bug

### 内部调整
- 无


## v1.5.0-rc.1

### 新增功能
- [[PR #28]](https://github.com/HITSZ-NRSL/firmware/pull/28) [fig] 1.添加fig/w32/w33 bootloader和应用
- [[PR #28]](https://github.com/HITSZ-NRSL/firmware/pull/28) [neutron/nut/fig] neutron/nut/fig 支持子系统版本升级

### 功能优化
- 无

### 漏洞修复
- [[PR #26]](https://github.com/HITSZ-NRSL/firmware/pull/26) [nut] 解决nut编译出现iram1_0_seg空间不足.
- [[PR #27]](https://github.com/HITSZ-NRSL/firmware/pull/27) [all] 1.解决usb串口配置，传输大数据指令无效. 2.减少配置等待时间.
- [[PR #34]](https://github.com/HITSZ-NRSL/firmware/pull/34) [all] 修正bug, 如果imlink配置成功，在进入配置模式配置，无法配置成功
- [[PR #35]](https://github.com/HITSZ-NRSL/firmware/pull/35) [nut] 修改nut tcp重连重启bug
- [[PR #35]](https://github.com/HITSZ-NRSL/firmware/pull/35) [nut/fig] 修改fig/nut/w323/w67 bootloader ota升级bug, 如果ota大小为0, 进入ota升级将陷入死循环

### 内部调整
- [[PR #35]](https://github.com/HITSZ-NRSL/firmware/pull/35) [all] 去掉sv_select相关处理


## v1.4.3-rc.2

### 新增功能
- 无

### 功能优化
- [[PR #19]](https://github.com/HITSZ-NRSL/firmware/pull/19) [neutron/nut] WiFi 增加status() 函数以兼容之前的固件

### 漏洞修复
- [[PR #22]](https://github.com/HITSZ-NRSL/firmware/pull/22) [neutron/nut/pwm] 解决 neutorn和nut的pwm 分辨率不能到16bit问题,解决pwm分辨率保存时互相错乱问题
- [[PR #19]](https://github.com/HITSZ-NRSL/firmware/pull/19) [neutron/pwm] 解决 neutorn的pwm 分辨率不可调问题

### 内部调整
- 无


## v1.4.3-rc.1

### 新增功能
- [neutron]新固件添加对neutron的支持
- [neutron]neutron添加获取wifi列表功能
- [nut]nut添加获取wifi列表功能
- [neutron/nut]添加用户与云端通用的接口，支持IntoYun服务器
- [neutron/nut]添加IntoYun注册和激活功能。

### 功能优化
- [neutorn]bootloader进入应用不初始化esp8266芯片，加快开机速度
- [all]去除固件所有警告信息
- [neutorn/nut]获取网络时间不从mqtt获取，直接通过网络ntp服务器获取

### 漏洞修复
- [nut]解决nut bootloader有时上电三色灯高亮的问题
- [neutorn]修正neutron运行一段时间后死机问题
- [neutorn]修正neutron运行一段时间后esp8266挂掉问题。neutron-net 更新至v2.0 sdk库
- [neutorn]neutron多任务处理，分为系统任务和应用任务，把应用任务对mqtt的处理和与modem通讯统一放入到system任务处理。以避免任务之间冲突

### 内部调整
- 无


