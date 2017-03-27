#include "hw_config.h"
#include "boot_mode.h"
#include "esp8266/esp8266_config.h"
#include "flash.h"
#include "memory_hal.h"
#include "params_hal.h"
#include "ui_hal.h"
#include "flash_map.h"
#include "boot_debug.h"

#define UPDATE_BLINK_PERIOD 100

void start_app(void)
{
    ETS_FRC1_INTR_DISABLE();
    if (!load_app(APP_ADDR)){
        SWRST;
    }

    while(true)
    {}
}

/*从flash中导入应用*/
bool load_app(const uint32_t flash_addr)
{
    image_header_t image_header;
    uint32_t pos = flash_addr;

    if (SPIRead(pos, (uint32_t*)&image_header, sizeof(image_header))){
        return false;
    }
    pos += sizeof(image_header);

    for (uint32_t section_index = 0;
        section_index < image_header.num_segments;
        ++section_index){
        section_header_t section_header = {0};
        if (SPIRead(pos, (uint32_t*)&section_header, sizeof(section_header))){
            return false;
        }
        pos += sizeof(section_header);

        const uint32_t address = section_header.address;
        bool load = false;

        if (address < 0x40000000){
            load = true;
        }

        if (address >= 0x40100000 && address < 0x40108000){
            load = true;
        }

        if (address >= 0x60000000){
            load = true;
        }

        if (!load){
            pos += section_header.size;
            continue;
        }

        if (SPIRead(pos, (uint32_t*)address, section_header.size)){
            return false;
        }

        pos += section_header.size;
    }
    register uint32_t sp asm("a1") = 0x3ffffff0;
    register uint32_t pc asm("a3") = image_header.entry;
    __asm__  __volatile__ ("jx a3");

    return true;
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
        if (SPIEraseSector(daddr/buffer_size)){
            return false;
        }
        if (SPIRead(saddr, (uint32_t*)buffer, buffer_size)){
            return false;
        }
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
    }
    else
    {
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
    if(DEFAULT_Flash_Reset())
    {
        HAL_PARAMS_Set_Boot_boot_flag(BOOT_FLAG_NORMAL);
        HAL_PARAMS_Save_Params();
    }
    else
    {
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
    if(DEFAULT_Flash_Reset())
    {
        HAL_PARAMS_Set_Boot_initparam_flag(INITPARAM_FLAG_FACTORY_RESET);
        HAL_PARAMS_Set_Boot_boot_flag(BOOT_FLAG_NORMAL);
        HAL_PARAMS_Save_Params();
    }
    else
    {
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
    if(OTA_Flash_Reset())
    {
        HAL_PARAMS_Set_Boot_boot_flag(BOOT_FLAG_NORMAL);
        HAL_PARAMS_Save_Params();
    }
    else
    {
        System_Reset();
    }
}

