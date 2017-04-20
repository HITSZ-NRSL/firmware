INCLUDE_DIRS += $(BOOTLOADER_TYPE_PATH)/inc

LDFLAGS += -Tlinker_$(PLATFORM_DEVICE_LC)_boot.ld
LDFLAGS += -L$(COMMON_BUILD)/linker/arm/$(PLATFORM_NAME)
LDFLAGS += -Wl,--defsym,__STACKSIZE__=400

LDFLAGS += -Wl,-Map,$(TARGET_BASE).map

ASRC_STARTUP += $(COMMON_BUILD)/startup/arm/$(PLATFORM_NAME)/startup_$(PLATFORM_DEVICE_LC).S
ASFLAGS += -I$(COMMON_BUILD)/startup/arm

LDFLAGS += --specs=nano.specs --specs=nosys.specs -lc -lnosys
LDFLAGS += -Wl,--start-group -lgcc -lc -Wl,--end-group

