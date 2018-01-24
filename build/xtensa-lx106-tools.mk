#
# 定义xtensa编译工具相关参数
#

# 定义编译器和工具的前缀

GCC_ARM_PATH ?= $(PROJECT_ROOT)/tools/xtensa-lx106-elf/bin/
GCC_PREFIX ?= xtensa-lx106-elf-

include $(COMMON_BUILD)/common-tools.mk

CDEFINES += -D__ets__ -DICACHE_FLASH -U__STRICT_ANSI__ -DF_CPU=80000000L -DARDUINO=10605 -DESP8266

# C 编译参数
ifneq ("$(MODULE)","bootloader")
CFLAGS += -Os
else
CFLAGS += -O0
CDEFINES += -DBOOTLOADER_BUILD=1
endif

CFLAGS += -g -w -Wall -mlongcalls -mtext-section-literals -Wfatal-errors -falign-functions=4 -MMD -ffunction-sections -fdata-sections -Wno-error=deprecated-declarations -fno-strict-aliasing -Wno-sign-compare

CONLYFLAGS += -Wpointer-arith -Wno-implicit-function-declaration -Wl,-EL -fno-inline-functions -nostdlib -std=gnu99

# C++ 编译参数  -fpermissive 把代码的语法错误作为警告,并继续编译进程
CPPFLAGS += -fno-exceptions -fno-rtti -std=c++11 -fpermissive

ASFLAGS += -g -x assembler-with-cpp -MMD -mlongcalls

LDFLAGS += -Os

# Board definitions
FLASH_SIZE ?= 16M    #此处必须是16M  此时用户参数区才会在1M 的最后16K上
FLASH_MODE ?= qio
FLASH_SPEED ?= 40

# Upload parameters
#UPLOAD_SPEED ?= 230400
UPLOAD_SPEED ?= 921600

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
UPLOAD_PORT ?= /dev/ttyUSB0
SUDO ?= sudo
endif

ifeq ($(UNAME_S),Darwin)
#UPLOAD_PORT ?= /dev/cu.usbmodem1411
UPLOAD_PORT ?= /dev/cu.SLAB_USBtoUART
endif

UPLOAD_VERB ?= -v
UPLOAD_RESET ?= nodemcu

