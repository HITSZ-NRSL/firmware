
# 定义编译工具前缀
GCC_ARM_PATH ?= $(PROJECT_ROOT)/tools/gcc-arm-none-eabi/bin/
GCC_PREFIX ?= arm-none-eabi-

include common-tools.mk
-include common-arm.mk

## other flags
CFLAGS += -mcpu=cortex-m4 -mfloat-abi=softfp -mfpu=fpv4-sp-d16
CFLAGS += -DSTM32F411xE
CFLAGS += -DUSE_HAL_DRIVER
CFLAGS += -DINTOROBOT_ARCH_ARM



# FLAGS For Neutron
CFLAGS += -I neutron/inc/hal/
CFLAGS += -I neutron/inc/platform/
CFLAGS += -I neutron/inc/services/
CFLAGS += -I neutron/inc/system/
CFLAGS += -I neutron/inc/user/
CFLAGS += -I neutron/inc/wiring/
CFLAGS += -I neutron/inc/wiring_ex/

LDFLAGS += -L neutron/lib/ -Wl,--whole-archive -lhal -lnewlib_nano -lplatform -lservices -lsystem -lwiring -lwiring_ex -lPDMFilter_CM4_GCC -Wl,--no-whole-archive
LDFLAGS += -L neutron/linker/
LDFLAGS += -T neutron/linker/linker_stm32f411xe_app.ld

NEWLIB_TWEAK_SPECS = neutron/linker/custom-nano.specs
LINKER_DEPS += $(NEWLIB_TWEAK_SPECS)
LDFLAGS += --specs=nano.specs --specs=$(NEWLIB_TWEAK_SPECS)
LDFLAGS += -Wl,--defsym,__STACKSIZE__=1400

STARTUP_OBJFILE += neutron/startup/startup_stm32f411xe.o

