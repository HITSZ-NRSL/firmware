#include <string.h>
#include <stdint.h>
#include <limits.h>

#include "boot_mode.h"
#include "memory_hal.h"
#include "system_config.h"
#include "params_hal.h"
#include "ui_hal.h"
#include "flash.h"
#include "flash_map.h"
#include "boot_debug.h"

extern "C"
{
#include "esp_err.h"
#include "esp_image_format.h"
#include "rom/cache.h"
#include "rom/rtc.h"
#include "soc/cpu.h"
#include "soc/dport_reg.h"
#include "bootloader_flash.h"
}

#define UPDATE_BLINK_PERIOD 100

/*从flash中导入应用*/
static void load_app(const uint32_t flash_addr)
{
    esp_err_t err;
    esp_image_header_t image_header;
    uint32_t image_length;

    /* TODO: verify the app image as part of OTA boot decision, so can have fallbacks */
    err = esp_image_basic_verify(flash_addr, true, &image_length);
    if (err != ESP_OK) {
        return;
    }

    if (esp_image_load_header(flash_addr, true, &image_header) != ESP_OK) {
        return;
    }

    uint32_t drom_addr = 0;
    uint32_t drom_load_addr = 0;
    uint32_t drom_size = 0;
    uint32_t irom_addr = 0;
    uint32_t irom_load_addr = 0;
    uint32_t irom_size = 0;

    /* Reload the RTC memory segments whenever a non-deepsleep reset
       is occurring */
    bool load_rtc_memory = rtc_get_reset_reason(0) != DEEPSLEEP_RESET;

    /* Important: From here on this function cannot access any global data (bss/data segments),
       as loading the app image may overwrite these.
    */
    for (int segment = 0; segment < image_header.segment_count; segment++) {
        esp_image_segment_header_t segment_header;
        uint32_t data_offs;
        if(esp_image_load_segment_header(segment, flash_addr,
                                         &image_header, true,
                                         &segment_header, &data_offs) != ESP_OK) {
            return;
        }

        const uint32_t address = segment_header.load_addr;
        bool load = true;
        bool map = false;
        if (address == 0x00000000) {        // padding, ignore block
            load = false;
        }
        if (address == 0x00000004) {
            load = false;                   // md5 checksum block
            // TODO: actually check md5
        }

        if (address >= DROM_LOW && address < DROM_HIGH) {
            drom_addr = data_offs;
            drom_load_addr = segment_header.load_addr;
            drom_size = segment_header.data_len + sizeof(segment_header);
            load = false;
            map = true;
        }

        if (address >= IROM_LOW && address < IROM_HIGH) {
            irom_addr = data_offs;
            irom_load_addr = segment_header.load_addr;
            irom_size = segment_header.data_len + sizeof(segment_header);
            load = false;
            map = true;
        }

        if (!load_rtc_memory && address >= RTC_IRAM_LOW && address < RTC_IRAM_HIGH) {
            load = false;
        }

        if (!load_rtc_memory && address >= RTC_DATA_LOW && address < RTC_DATA_HIGH) {
            load = false;
        }

        if (load) {
            intptr_t sp, start_addr, end_addr;

            start_addr = segment_header.load_addr;
            end_addr = start_addr + segment_header.data_len;

            /* Before loading segment, check it doesn't clobber
               bootloader RAM... */

            if (end_addr < 0x40000000) {
                sp = (intptr_t)get_sp();
                if (end_addr > sp) {
                    return;
                }
                if (end_addr > sp - 256) {
                }
            }

            const void *data = bootloader_mmap(data_offs, segment_header.data_len);
            if(!data) {
                return;
            }
            memcpy((void *)segment_header.load_addr, data, segment_header.data_len);
            bootloader_munmap(data);
        }
    }

    //set cache and start app
    Cache_Read_Disable( 0 );
    Cache_Flush( 0 );
    uint32_t drom_page_count = (drom_size + 64*1024 - 1) / (64*1024); // round up to 64k
    int rc = cache_flash_mmu_set( 0, 0, drom_load_addr & 0xffff0000, drom_addr & 0xffff0000, 64, drom_page_count );
    rc = cache_flash_mmu_set( 1, 0, drom_load_addr & 0xffff0000, drom_addr & 0xffff0000, 64, drom_page_count );
    uint32_t irom_page_count = (irom_size + 64*1024 - 1) / (64*1024); // round up to 64k
    rc = cache_flash_mmu_set( 0, 0, irom_load_addr & 0xffff0000, irom_addr & 0xffff0000, 64, irom_page_count );
    rc = cache_flash_mmu_set( 1, 0, irom_load_addr & 0xffff0000, irom_addr & 0xffff0000, 64, irom_page_count );
    REG_CLR_BIT( DPORT_PRO_CACHE_CTRL1_REG, (DPORT_PRO_CACHE_MASK_IRAM0) | (DPORT_PRO_CACHE_MASK_IRAM1 & 0) | (DPORT_PRO_CACHE_MASK_IROM0 & 0) | DPORT_PRO_CACHE_MASK_DROM0 | DPORT_PRO_CACHE_MASK_DRAM1 );
    REG_CLR_BIT( DPORT_APP_CACHE_CTRL1_REG, (DPORT_APP_CACHE_MASK_IRAM0) | (DPORT_APP_CACHE_MASK_IRAM1 & 0) | (DPORT_APP_CACHE_MASK_IROM0 & 0) | DPORT_APP_CACHE_MASK_DROM0 | DPORT_APP_CACHE_MASK_DRAM1 );
    Cache_Read_Enable( 0 );

    // Application will need to do Cache_Flush(1) and Cache_Read_Enable(1)
    //ESP_LOGD(TAG, "start: 0x%08x", image_header.entry_addr);
    typedef void (*entry_t)(void);
    entry_t entry = ((entry_t) image_header.entry_addr);

    // TODO: we have used quite a bit of stack at this point.
    // use "movsp" instruction to reset stack back to where ROM stack starts.
    (*entry)();
}

void start_app(void)
{
    load_app(APP_ADDR);

    while(true)
    {}
}


/*从flash起始地址复制至目标地址*/
bool copy_raw(const uint32_t src_addr, const uint32_t dst_addr, const uint32_t size)
{
    // require regions to be aligned
    if (src_addr & 0xfff != 0 ||
        dst_addr & 0xfff != 0){
        return false;
    }

    const uint32_t buffer_size = FLASH_SECTOR_SIZE;
    uint8_t buffer[buffer_size];
    uint32_t left = ((size+buffer_size-1) & ~(buffer_size-1));
    uint32_t saddr = src_addr;
    uint32_t daddr = dst_addr;

    while (left){
        RGB_Color_Toggle();
        if (SPIEraseSector(daddr/buffer_size)){
            return false;
        }
        RGB_Color_Toggle();
        if (SPIRead(saddr, (uint32_t*)buffer, buffer_size)){
            return false;
        }
        RGB_Color_Toggle();
        if (SPIWrite(daddr, (uint32_t*)buffer, buffer_size)){
            return false;
        }
        saddr += buffer_size;
        daddr += buffer_size;
        left  -= buffer_size;
    }

    return true;
}
extern "C"
{
    extern uint32_t HAL_PARAMS_Get_Boot_ota_app_size(void);
    extern uint32_t HAL_PARAMS_Get_Boot_def_app_size(void);
}

bool FLASH_Restore(Firmware_TypeDef FmType)
{
    bool result;
    uint32_t size;

    if(DEFAULT_FIRWARE == FmType) {
        size = HAL_PARAMS_Get_Boot_def_app_size();
        if(0 == size) {
            size = DEFAULT_APP_SEC_NUM*SPI_FLASH_SEC_SIZE;
        }
        result = copy_raw(DEFAULT_APP_ADDR, APP_ADDR, size);
    } else {
        size = HAL_PARAMS_Get_Boot_ota_app_size();
        if(0 == size) {
            return true;
        }
        result = copy_raw(CACHE_ONLINE_APP_ADDR, APP_ADDR, size);
    }
    return result;
}

bool DEFAULT_Flash_Reset(void)
{
    return FLASH_Restore(DEFAULT_FIRWARE);
}

bool OTA_Flash_Reset(void)
{
    return FLASH_Restore(OTA_FIRWARE);
}

void Enter_Default_RESTORE_Mode(void)
{
    HAL_UI_RGB_Blink(RGB_COLOR_YELLOW, UPDATE_BLINK_PERIOD);
    if(DEFAULT_Flash_Reset()) {
        HAL_PARAMS_Set_Boot_boot_flag(BOOT_FLAG_NORMAL);
        HAL_PARAMS_Save_Params();
    } else {
        System_Reset();
    }
}

void Enter_Serail_Com_Mode(void)
{
    HAL_PARAMS_Set_Boot_boot_flag(BOOT_FLAG_NORMAL);
    HAL_PARAMS_Save_Params();
    System_Reset();
}

void Enter_Factory_RESTORE_Mode(void)
{
    HAL_UI_RGB_Blink(RGB_COLOR_YELLOW, UPDATE_BLINK_PERIOD);
    if(DEFAULT_Flash_Reset()) {
        HAL_PARAMS_Set_Boot_initparam_flag(INITPARAM_FLAG_FACTORY_RESET);
        HAL_PARAMS_Set_Boot_boot_flag(BOOT_FLAG_NORMAL);
        HAL_PARAMS_Save_Params();
    } else {
        System_Reset();
    }
}

void Enter_Factory_ALL_RESTORE_Mode(void)
{
    HAL_UI_RGB_Blink(RGB_COLOR_YELLOW, UPDATE_BLINK_PERIOD);
    delay(1000);
    HAL_PARAMS_Set_Boot_initparam_flag(INITPARAM_FLAG_ALL_RESET);
    HAL_PARAMS_Set_Boot_boot_flag(BOOT_FLAG_NORMAL);
    HAL_PARAMS_Save_Params();
}

void Enter_OTA_Update_Mode(void)
{
    HAL_UI_RGB_Blink(RGB_COLOR_YELLOW, UPDATE_BLINK_PERIOD);
    if(OTA_Flash_Reset()) {
        HAL_PARAMS_Set_Boot_boot_flag(BOOT_FLAG_NORMAL);
        HAL_PARAMS_Save_Params();
    } else {
        System_Reset();
    }
}

