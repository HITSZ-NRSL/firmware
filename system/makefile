
MODULE=system
SYSTEM_MODULE_PATH=.
BUILD_PATH_EXT = $(SYSTEM_BUILD_PATH_EXT)

DEPENDENCIES = hal services wiring wiring_ex
ifeq ("","$(INTOROBOT_NO_PLATFORM)")
DEPENDENCIES += platform
CFLAGS += -DINTOROBOT_PLATFORM
endif
TARGET_TYPE = a


include ../build/arm-tlm.mk
