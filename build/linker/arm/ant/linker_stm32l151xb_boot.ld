/*
*****************************************************************************
**
**  File        : linker_stm32l151xb_app.ld
**
**  Abstract    : Linker script for STM32L151XB Device with
**                128KByte FLASH, 16KByte SRAM
**
**                Set heap size, stack size and stack location according
**                to application requirements.
**
**                Set memory bank area and size if external memory is used.
**
**  Target      : STMicroelectronics STM32
**
**  Environment : Eclipse-CDT and GNU Tools ARM Embedded.
**
*****************************************************************************
*/

/* default stack sizes.

These are used by the startup in order to allocate stacks for the different modes.
*/

__Stack_Size = 2048 ;

__Stack_Init = _estack - __Stack_Size ;

/*
There will be a link error if there is not this amount of RAM free at the end.
*/
_Minimum_Stack_Size = 0x400;

/* include the memory spaces definitions sub-script */
/*
Linker subscript for STM32L151XB definitions with 128K Flash and 16K RAM
*/

/* Memory Spaces Definitions */

MEMORY
{
    SRAM (xrw)        : ORIGIN = 0x20000000, LENGTH = 16K
    APP_FLASH (rx)    : ORIGIN = 0x08000000, LENGTH = 20K
}

INCLUDE linker_stm32l151xb_common.ld