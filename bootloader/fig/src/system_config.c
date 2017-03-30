#include "flash_map.h"
#include "rtc.h"
#include "sdkconfig.h"
#include "esp_image_format.h"
#include "rom/spi_flash.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/timer_group_reg.h"
#include "soc/dport_reg.h"
#include "soc/timer_group_struct.h"
#include "esp_intr_alloc.h"
#include "esp_intr.h"
#include "esp_flash_partitions.h"
#include "boot_debug.h"
#include "ui_hal.h"

/* #include "xtensa_intr_asm.S" */

/**
 * @brief  System Clock Configuration
 * @param  96M
 * @retval None
 */
void SystemClock_Config(void)
{
    rtc_set_cpu_freq(CPU_XTAL);
    rtc_set_cpu_freq(CPU_80M);
}
/**
 * @brief  This function handles SysTick Handler.
 * @param  None
 * @retval None
 */
void SysTick_Handler(void)
{
    HAL_UI_SysTick_Handler();
}

static void uart_console_configure(void)
{
    const int uart_num = CONFIG_CONSOLE_UART_NUM;

    uartAttach();
    ets_install_uart_printf();

    // ROM bootloader may have put a lot of text into UART0 FIFO.
    // Wait for it to be printed.
    uart_tx_wait_idle(0);

    // Set configured UART console baud rate
    const int uart_baud = CONFIG_CONSOLE_UART_BAUDRATE;
    uart_div_modify(uart_num, (APB_CLK_FREQ << 4) / uart_baud);
}

/**
 * @brief  update flash config
 * @param  96M
 * @retval None
 */
static void Update_flash_config(void)
{
    esp_image_header_t fhdr;
    uint32_t size;

    if(esp_image_load_header(APP_ADDR, true, &fhdr) != ESP_OK) {
        return;
    }
    SPIUnlock();

    switch(fhdr.spi_size) {
        case ESP_IMAGE_FLASH_SIZE_1MB:
            size = 1;
            break;
        case ESP_IMAGE_FLASH_SIZE_2MB:
            size = 2;
            break;
        case ESP_IMAGE_FLASH_SIZE_4MB:
            size = 4;
            break;
        case ESP_IMAGE_FLASH_SIZE_8MB:
            size = 8;
            break;
        case ESP_IMAGE_FLASH_SIZE_16MB:
            size = 16;
            break;
        default:
            size = 2;
    }
    Cache_Read_Disable( 0 );
    // Set flash chip size
    SPIParamCfg(g_rom_flashchip.deviceId, size * 0x100000, 0x10000, 0x1000, 0x100, 0xffff);
    // TODO: set mode
    // TODO: set frequency
    Cache_Flush(0);
    Cache_Read_Enable( 0 );
}

static bool load_partition_table(void)
{
    const esp_partition_info_t *partitions;
    const int ESP_PARTITION_TABLE_DATA_LEN = 0xC00; /* length of actual data (signature is appended to this) */
    char *partition_usage;
    esp_err_t err;
    int num_partitions;

    partitions = bootloader_mmap(ESP_PARTITION_TABLE_ADDR, ESP_PARTITION_TABLE_DATA_LEN);
    if (!partitions) {
        return false;
    }

    BOOT_DEBUG("mapped partition table 0x%x at 0x%x\r\n", ESP_PARTITION_TABLE_ADDR, (intptr_t)partitions);

    err = esp_partition_table_basic_verify(partitions, true, &num_partitions);
    if (err != ESP_OK) {
        BOOT_DEBUG("Failed to verify partition table\r\n");
        return false;
    }
    BOOT_DEBUG("Partition Table:\r\n");
    BOOT_DEBUG("## Label            Usage          Type ST Offset   Length\r\n");

    for(int i = 0; i < num_partitions; i++) {
        const esp_partition_info_t *partition = &partitions[i];
        BOOT_DEBUG("load partition table entry 0x%x\r\n", (intptr_t)partition);
        BOOT_DEBUG("type=%x subtype=%x\r\n", partition->type, partition->subtype);
        partition_usage = "unknown";

        /* valid partition table */
        switch(partition->type) {
        case PART_TYPE_APP: /* app partition */
            switch(partition->subtype) {
            case PART_SUBTYPE_FACTORY: /* factory binary */
                partition_usage = "factory app";
                break;
            case PART_SUBTYPE_TEST: /* test binary */
                partition_usage = "test app";
                break;
            default:
                /* OTA binary */
                if ((partition->subtype & ~PART_SUBTYPE_OTA_MASK) == PART_SUBTYPE_OTA_FLAG) {
                    partition_usage = "OTA app";
                }
                else {
                    partition_usage = "Unknown app";
                }
                break;
            }
            break; /* PART_TYPE_APP */
        case PART_TYPE_DATA: /* data partition */
            switch(partition->subtype) {
            case PART_SUBTYPE_DATA_OTA: /* ota data */
                partition_usage = "OTA data";
                break;
            case PART_SUBTYPE_DATA_RF:
                partition_usage = "RF data";
                break;
            case PART_SUBTYPE_DATA_WIFI:
                partition_usage = "WiFi data";
                break;
            default:
                partition_usage = "Unknown data";
                break;
            }
            break; /* PARTITION_USAGE_DATA */
        default: /* other partition type */
            break;
        }

        /* print partition type info */
        BOOT_DEBUG("%2d %-16s %-16s %02x %02x %08x %08x\r\n", i, partition->label, partition_usage,
                 partition->type, partition->subtype,
                 partition->pos.offset, partition->pos.size);
    }

    bootloader_munmap(partitions);
    BOOT_DEBUG("End of partition table\r\n");
    return true;
}

/**
 * @brief  Configures Main system clocks & power.
 * @param  None
 * @retval None
 */
void Set_System(void)
{
    SystemClock_Config();
    uart_console_configure();
    /* disable watch dog here */
    REG_CLR_BIT( RTC_CNTL_WDTCONFIG0_REG, RTC_CNTL_WDT_FLASHBOOT_MOD_EN );
    REG_CLR_BIT( TIMG_WDTCONFIG0_REG(0), TIMG_WDT_FLASHBOOT_MOD_EN );
    bootloader_random_enable();
    Update_flash_config();
    load_partition_table();
    bootloader_random_disable();

    HAL_UI_Initial();
}

void delay(uint32_t ms)
{
    ets_delay_us(ms*1000);
}

void System_Reset(void)
{
    _ResetVector();
}

/* empty rtc_printf implementation, to work with librtc
   linking. Can be removed once -lrtc is removed from bootloader's
   main component.mk.
*/
int rtc_printf(void)
{
    return 0;
}
