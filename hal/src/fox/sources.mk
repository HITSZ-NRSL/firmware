
TARGET_HAL_SRC_PATH = $(TARGET_HAL_PATH)/src/$(PLATFORM_NAME)

CSRC += $(call here_files,$(TARGET_HAL_SRC_PATH)/,*.c)

CPPSRC += $(call here_files,$(TARGET_HAL_SRC_PATH)/,*.cpp)

# ASM source files included in this build.
ASRC += $(call here_files,$(TARGET_HAL_SRC_PATH)/,*.S)

include $(call rwildcard,$(TARGET_HAL_STM32F4XX_SHARE_PATH)/,sources.mk)

