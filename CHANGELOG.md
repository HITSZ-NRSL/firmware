## v1.5.0-rc.2

### FEATURES(功能)

### ENHANCEMENTS(优化)

### BUG FIXES(漏洞修复)
- [[PR #42]](https://github.com/HITSZ-NRSL/firmware/pull/42) [nut] 修改nut jsonGetValue()重启bug

### INTERNAL(内部)
- [[PR #43]](https://github.com/HITSZ-NRSL/firmware/pull/43) [anytest] 增加各板子测试代码

## v1.5.0-rc.1

### FEATURES(功能)
- [[PR #28]](https://github.com/HITSZ-NRSL/firmware/pull/28) [fig] 1.添加fig/w32/w33 bootloader和应用
- [[PR #28]](https://github.com/HITSZ-NRSL/firmware/pull/28) [neutron/nut/fig] neutron/nut/fig 支持子系统版本升级

### ENHANCEMENTS(优化)


### BUG FIXES(漏洞修复)
- [[PR #26]](https://github.com/HITSZ-NRSL/firmware/pull/26) [nut] 解决nut编译出现iram1_0_seg空间不足.
- [[PR #27]](https://github.com/HITSZ-NRSL/firmware/pull/27) [all] 1.解决usb串口配置，传输大数据指令无效. 2.减少配置等待时间.
- [[PR #34]](https://github.com/HITSZ-NRSL/firmware/pull/34) [all] 修正bug, 如果imlink配置成功，在进入配置模式配置，无法配置成功
- [[PR #35]](https://github.com/HITSZ-NRSL/firmware/pull/35) [nut] 修改nut tcp重连重启bug
- [[PR #35]](https://github.com/HITSZ-NRSL/firmware/pull/35) [nut/fig] 修改fig/nut/w323/w67 bootloader ota升级bug, 如果ota大小为0, 进入ota升级将陷入死循环

### INTERNAL(内部)
- [[PR #35]](https://github.com/HITSZ-NRSL/firmware/pull/35) [all] 去掉sv_select相关处理



## v1.4.3-rc.2

### FEATURES(功能)


### ENHANCEMENTS(优化)
- [[PR #19]](https://github.com/HITSZ-NRSL/firmware/pull/19) [neutron/nut] WiFi 增加status() 函数以兼容之前的固件


### BUG FIXES(漏洞修复)
- [[PR #22]](https://github.com/HITSZ-NRSL/firmware/pull/22) [neutron/nut/pwm] 解决 neutorn和nut的pwm 分辨率不能到16bit问题,解决pwm分辨率保存时互相错乱问题
- [[PR #19]](https://github.com/HITSZ-NRSL/firmware/pull/19) [neutron/pwm] 解决 neutorn的pwm 分辨率不可调问题


### INTERNAL(内部)



## v1.4.3-rc.1

### FEATURES(功能)

- [neutron]新固件添加对neutron的支持
- [neutron]neutron添加获取wifi列表功能
- [nut]nut添加获取wifi列表功能
- [neutron/nut]添加用户与云端通用的接口，支持IntoYun服务器
- [neutron/nut]添加IntoYun注册和激活功能。

### ENHANCEMENTS(优化)

- [neutorn]bootloader进入应用不初始化esp8266芯片，加快开机速度
- [all]去除固件所有警告信息
- [neutorn/nut]获取网络时间不从mqtt获取，直接通过网络ntp服务器获取

### BUG FIXES(漏洞修复)

- [nut]解决nut bootloader有时上电三色灯高亮的问题
- [neutorn]修正neutron运行一段时间后死机问题
- [neutorn]修正neutron运行一段时间后esp8266挂掉问题。neutron-net 更新至v2.0 sdk库
- [neutorn]neutron多任务处理，分为系统任务和应用任务，把应用任务对mqtt的处理和与modem通讯统一放入到system任务处理。以避免任务之间冲突

### INTERNAL(内部)


## v1.4.1

### FEATURES(功能)


### ENHANCEMENTS(优化)


### BUG FIXES(漏洞修复)


### INTERNAL(内部)


