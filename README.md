# firmware
IntoRobot 固件源代码工程, 支持intorobot创客模块和IntoRobot商业模组。

```
创客核心板（包括三色灯、按键、usb接口等）：atom, neutron, nut, ant, fig, fox等。
商业模组（单纯模组，不包括其他附属器件）：w6, w7, w32, w33, l6等。
```
## 快速开始
### 通过git下载仓库代码
打开终端，进入到目的路径，输入下面命令：

`git clone https://github.com/HITSZ-NRSL/firmware.git`

如果未安装git，请参考相关资料进行安装并配置。

### 安装编译和烧录工具

安装需要的编译和烧录工具，包括arm-none-eabi-gcc, xtensa-lx106, xtensa-esp32, st-flash, dfu-util、esptool、esptool-py。运行以下命令就可以安装：

```
cd tools
sudo ./install-tools.sh
```

安装脚本会自动辨别PC操作系统类型，下载相应的工具。

### 编译与烧录

在firmware目录下，可以进行各个产品的编译。编译默认固件的命令主要如下：

```
make PLATFORM=neutron clean all APP=smartLight-default
make PLATFORM=nut clean all APP=smartLight-default
make PLATFORM=atom clean all APP=smartLight-default
make PLATFORM=fig clean all APP=smartLight-default
make PLATFORM=ant clean all APP=blink
make PLATFORM=fox clean all APP=smartLight-default

make PLATFORM=w67 clean all APP=smartLight-w67
make PLATFORM=w323 clean all APP=blink
make PLATFORM=l6 clean all APP=blink
```


其中，*PLATFORM=product_name*也可以替换成*PLATFORM_ID=product_id*.产品的名称和ID的关系如下表（详情请参见build/platform-id.mk）：

| 创客核心板   | PLATFORM_ID |
|--------------|:-----------:|
| neutron      | 888002      |
| nut          | 888003      |
| atom         | 888004      |
| fig          | 888005      |
| ant          | 888006      |
| fox          | 888007      |

| 商业模块     | PLATFORM_ID |
|--------------|:-----------:|
| w67          | 888101      |
| w323         | 888102      |
| l6           | 888103      |

| 其他产品     | PLATFORM_ID |
|--------------|:-----------:|
| gcc          | 888201      |
| neutron-net  | 888202      |
| anytest      | 888203      |

进入到main目录下，可以选择更多的编译选项，还可以进行烧录。
以下常用的编译及烧录命令：

```
make PLATFORM=atom clean all st-flash
make PLATFORM=atom clean all program-dfu

make PLATFORM=neutron clean all st-flash
make PLATFORM=neutron clean all program-dfu

make PLATFORM=ant clean all DEBUG_BUILD=y USE_SWD=y st-flash
make PLATFORM=ant clean all DEBUG_BUILD=y USE_SWD=y program-dfu

make PLATFORM=fox clean all DEBUG_BUILD=y USE_SWD=y st-flash
make PLATFORM=fox clean all DEBUG_BUILD=y USE_SWD=y program-dfu

make PLATFORM=nut clean all DEBUG_BUILD=y USE_SWD=y esptool

make PLATFORM=fig clean all DEBUG_BUILD=y USE_SWD=y esptool-py

make PLATFORM=w67 clean all DEBUG_BUILD=y USE_SWD=y esptool

make PLATFORM=w323 clean all DEBUG_BUILD=y USE_SWD=y esptool-py

make PLATFORM=l6 clean all DEBUG_BUILD=y USE_SWD=y st-flash
make PLATFORM=l6 clean all DEBUG_BUILD=y USE_SWD=y program-dfu

```

DEBUG_BUILD=y打开调试， st-flash program-dfu esptool分别选择相应的烧录工具。

## 代码架构

代码主要是以如下形式组织:

| 模块 | 简介 |
|---:|:---|
| **bootloader** | 各个产品的bootloader代码 |
| **build**      | 编译目录，里面包括编译的主要makefile，以及链接文件，编译的目标文件 |
| **docs**       | 一些相关的文档 |
| **hal**        | 硬件描述层接口 |
| **main**       | 编译烧录各个固件的上层文件夹 |
| **newlib_nano**| 主要用于操作系统内存管理保护 |
| **platform**   | 各个产品芯片的提供的库，属于最底层代码 |
| **services**   | 主要是调试功能 |
| **system**     | 提供连接网络，配置，连接intorobot平台，在线编程，固件更新等功能 |
| **tools**      | 编译和烧录工具 |
| **user**       | 包括各个产品的默认应用代码， 各种应用代码，外部库，以及自动测试代码 |
| **wiring**     | Arduino兼容功能接口， 主要通过调用hal层接口 |
| **wiring_ex**  | 针对于特定核心板扩展的函数接口， 主要通过调用hal层接口 |

### bootloader

实现产品的bootloader, bootloader中实现应用程序的引导，DFU功能实现，恢复默认固件，恢复出厂设置等功能。

### build

实现编译的主体:

arm-xxx.mk实现包括atom, neutron, ant, fox, l6等的编译。

xtensa-lx106-xxx.mk实现esp8266相关的编译，包括neutron-net,nut, w67等。

xtensa-esp32-xxx.mk实现esp32相关的编译，包括fig, w323等。

linker存放链接文件，startup包含启动文件。

编译的的目标文件会放在target目录里面的相应模块名称和PLATFORM_ID文件夹中。

### docs

其他文档文件。

### hal

硬件描述层接口，它调用platform模块的代码，实现了各个底层接口。这些接口被bootloader, services, system, wiring, wiring_ex模块调用。用户如果新增产品，需要在这里实现相应的接口。

### main

目前仅提供编译烧录功能，提过了其他编译选项。编译详情参见docs/debugging.md。

### newlib_nano

主要用于操作系统内存管理保护, neutorn使用到

### platform

主要放置产品芯片厂家提供的库，包括ESP8266,ESP32,STM32F1xx, STM32F4xx, STM32L1xx等。

### services

提供调试接口等辅助服务功能。

### system

提供连接网络,配置，连接intorobot平台，在线编程，固件更新等功能。

### tools

通过install-tools.sh脚本，安装需要的编译和烧录工具，这些工具都存放在这个目录里面。

### user

application文件夹中放了默认的产品应用例程， 也可以放自己的应用例程。用到的外部库文件，放在libraries。tests文件夹中放代码测试功能，主要测试wiring层的功能是否健全。

### wiring

里面提供了Arduino的兼容函数接口, 函数接口的含义请参见[IntoRobot官方资料](http://docs.intorobot.com/interface/base-firmware/)。

### wiring_ex

里面提供了针对于特定核心板扩展的函数接口, 函数接口的含义请参见[IntoRobot官方资料](http://docs.intorobot.com/interface/base-firmware/)。
