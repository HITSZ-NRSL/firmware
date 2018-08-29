/**
 ******************************************************************************
 * @file    USB_Device/DFU_Standalone/Src/usbd_dfu_flash.c
 * @author  MCD Application Team
 * @version V1.3.2
 * @date    13-November-2015
 * @brief   Memory management layer
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2015 STMicroelectronics</center></h2>
 *
 * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *        http://www.st.com/software_license_agreement_liberty_v2
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "usbd_dfu_if.h"
#include "stm32l1xx_hal_conf.h"


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define FLASH_DESC_STR      "@Internal Flash   /0x08000000/20*01Ka,108*01Kg"
#define FLASH_ERASE_TIME    (uint16_t)50
#define FLASH_PROGRAM_TIME  (uint16_t)50

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static uint32_t GetSector(uint32_t Address);

/* Extern function prototypes ------------------------------------------------*/
uint16_t Flash_If_Init(void);
uint16_t Flash_If_Erase(uint32_t Add);
uint16_t Flash_If_Write(uint8_t *src, uint8_t *dest, uint32_t Len);
uint8_t *Flash_If_Read(uint8_t *src, uint8_t *dest, uint32_t Len);
uint16_t Flash_If_DeInit(void);
uint16_t Flash_If_GetStatus(uint32_t Add, uint8_t Cmd, uint8_t *buffer);


extern void print(char *p);

#if defined ( __ICCARM__ ) /*!< IAR Compiler */
#pragma data_alignment=4
#endif
__ALIGN_BEGIN USBD_DFU_MediaTypeDef USBD_DFU_fops __ALIGN_END = {
    (uint8_t *)FLASH_DESC_STR,
    Flash_If_Init,
    Flash_If_DeInit,
    Flash_If_Erase,
    Flash_If_Write,
    Flash_If_Read,
    Flash_If_GetStatus,
};

/* Private functions ---------------------------------------------------------*/
/**
 * @brief  Initializes Memory.
 * @param  None
 * @retval 0 if operation is successeful, MAL_FAIL else.
 */
uint16_t Flash_If_Init(void)
{
    /* Unlock the internal flash */
    HAL_FLASH_Unlock();
    return 0;
}

/**
 * @brief  De-Initializes Memory.
 * @param  None
 * @retval 0 if operation is successeful, MAL_FAIL else.
 */
uint16_t Flash_If_DeInit(void)
{
    /* Lock the internal flash */
    HAL_FLASH_Lock();
    return 0;
}

/**
 * @brief  Erases sector.
 * @param  Add: Address of sector to be erased.
 * @retval 0 if operation is successeful, MAL_FAIL else.
 */
uint16_t Flash_If_Erase(uint32_t Add)
{
    uint32_t NbOfPages = 0;
    uint32_t PageError = 0;
    /* Variable contains Flash operation status */
    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef eraseinitstruct;

    /* Get the number of sector to erase from 1st sector*/
    eraseinitstruct.TypeErase = FLASH_TYPEERASE_PAGES;
    eraseinitstruct.PageAddress = Add;
    eraseinitstruct.NbPages = 4;
    status = HAL_FLASHEx_Erase(&eraseinitstruct, &PageError);

    if (status != HAL_OK)
    {
        return 1;
    }
    return 0;
}

/**
 * @brief  Writes Data into Memory.
 * @param  src: Pointer to the source buffer. Address to be written to.
 * @param  dest: Pointer to the destination buffer.
 * @param  Len: Number of data to be written (in bytes).
 * @retval 0 if operation is successeful, MAL_FAIL else.
 */
uint16_t Flash_If_Write(uint8_t *src, uint8_t *dest, uint32_t Len)
{
    uint32_t i = 0;
    __IO uint8_t memBuffer[128];

    for(i = 0; i < Len; i += 128) {
        memset(memBuffer, 0, sizeof(memBuffer));
        if((i + 128) < Len) {
            memcpy(memBuffer, src+i, 128);
        } else {
            memcpy(memBuffer, src+i, Len - i);
        }
        if(FLASH_HalfPageProgram((uint32_t)(dest+i), (uint32_t*)memBuffer) != HAL_OK ) {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief  Reads Data into Memory.
 * @param  src: Pointer to the source buffer. Address to be written to.
 * @param  dest: Pointer to the destination buffer.
 * @param  Len: Number of data to be read (in bytes).
 * @retval Pointer to the physical address where data should be read.
 */
uint8_t *Flash_If_Read(uint8_t *src, uint8_t *dest, uint32_t Len)
{
    uint32_t i = 0;
    uint8_t *psrc = src;

    for(i = 0; i < Len; i++)
    {
        dest[i] = *psrc++;
    }
    /* Return a valid address to avoid HardFault */
    return (uint8_t*)(dest);
}

/**
 * @brief  Gets Memory Status.
 * @param  Add: Address to be read from.
 * @param  Cmd: Number of data to be read (in bytes).
 * @retval 0 if operation is successeful
 */
uint16_t Flash_If_GetStatus(uint32_t Add, uint8_t Cmd, uint8_t *buffer)
{
    switch(Cmd)
    {
        case DFU_MEDIA_PROGRAM:
            buffer[1] = (uint8_t)FLASH_PROGRAM_TIME;
            buffer[2] = (uint8_t)(FLASH_PROGRAM_TIME << 8);
            buffer[3] = 0;
            break;

        case DFU_MEDIA_ERASE:
        default:
            buffer[1] = (uint8_t)FLASH_ERASE_TIME;
            buffer[2] = (uint8_t)(FLASH_ERASE_TIME << 8);
            buffer[3] = 0;
            break;
    }
    return 0;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
