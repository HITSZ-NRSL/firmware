# This file is a makefile included from the top level makefile which
# defines the sources built for the target.

# Define the prefix to this directory.
# Note: The name must be unique within this build and should be
#       based on the root of the project

TARGET_WIRING_EX_SRC_PATH = $(WIRING_EX_MODULE_PATH)/$(PLATFORM_NAME)/src

INCLUDE_DIRS += $(TARGET_WIRING_EX_SRC_PATH)

# C source files included in this build.
CSRC += $(call target_files,$(TARGET_WIRING_EX_SRC_PATH)/,*.c)

# enumerate target cpp files
CPPSRC += $(call target_files,$(TARGET_WIRING_EX_SRC_PATH)/,*.cpp)

# ASM source files included in this build.
ASRC +=

