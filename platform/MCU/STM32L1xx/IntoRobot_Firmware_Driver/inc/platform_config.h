/**
 ******************************************************************************
 * @file    platform_config.h
 * @authors Satish Nair, Brett Walach
 * @version V1.0.0
 * @date    22-Oct-2014
 * @brief   Board specific configuration file.
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __PLATFORM_CONFIG_H
#define __PLATFORM_CONFIG_H

#include "platforms.h"

#ifndef PLATFORM_ID
#error "PLATFORM_ID not defined"
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l1xx.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

#if PLATFORM_GPRS == PLATFORM_ID
    #define HAS_SERIAL_FLASH
    #define USE_SERIAL_FLASH
    #define sFLASH_PAGESIZE     0x1000 /* 4096 bytes sector size that needs to be erased */
    #define sFLASH_PAGECOUNT    512    /* 2MByte storage */
#endif

#ifdef HAS_SERIAL_FLASH
//SPI FLASH Interface pins
#define sFLASH_SPI                          SPI2
#define sFLASH_SPI_CS_GPIO_PIN              GPIO_PIN_12                 /* PB.12 */
#define sFLASH_SPI_CS_GPIO_PORT             GPIOB                       /* GPIOB */
#define sFLASH_SPI_SCK_GPIO_PIN             GPIO_PIN_13                 /* PB.13 */
#define sFLASH_SPI_SCK_GPIO_PORT            GPIOB                       /* GPIOB */
#define sFLASH_SPI_SCK_AF                   GPIO_AF5_SPI2
#define sFLASH_SPI_MISO_GPIO_PIN            GPIO_PIN_14                 /* PB.14 */
#define sFLASH_SPI_MISO_GPIO_PORT           GPIOB                       /* GPIOB */
#define sFLASH_SPI_MISO_AF                  GPIO_AF5_SPI2
#define sFLASH_SPI_MOSI_GPIO_PIN            GPIO_PIN_15                 /* PB.15 */
#define sFLASH_SPI_MOSI_GPIO_PORT           GPIOB                       /* GPIOB */
#define sFLASH_SPI_MOSI_AF                  GPIO_AF5_SPI2

#define sFLASH_SPI_BAUDRATE_PRESCALER       SPI_BAUDRATEPRESCALER_256
#endif

//NVIC Priorities based on NVIC_PriorityGroup_4
#define SDIO_IRQ_PRIORITY                   0       //??? BCM43362 SDIO Interrupt
#define OTG_LP_IRQ_PRIORITY                 2       //USB OTG FS Interrupt
#define OTG_LP_WKUP_IRQ_PRIORITY            2       //USB OTG FS Wakeup Interrupt
#define RTC_Alarm_IRQ_PRIORITY              3       //RTC Alarm Interrupt
#define RTC_WKUP_IRQ_PRIORITY               4       //RTC Seconds Interrupt
#define USART1_IRQ_PRIORITY                 5       //USART1 Interrupt
#define USART2_IRQ_PRIORITY                 5       //USART2 Interrupt
#define TIM_IRQ_PRIORITY                    6       //TIM Elapsed Interrupt
//BUTTON1_EXTI_IRQ_PRIORITY                 7       //Mode Button Interrupt
#define EXTI15_10_IRQ_PRIORITY              8       //User Interrupt
#define EXTI9_5_IRQ_PRIORITY                9       //User Interrupt
#define EXTI0_IRQ_PRIORITY                  10      //User Interrupt
#define EXTI1_IRQ_PRIORITY                  10      //User Interrupt
#define EXTI2_IRQ_PRIORITY                  10      //User Interrupt
#define EXTI3_IRQ_PRIORITY                  10      //User Interrupt
#define EXTI4_IRQ_PRIORITY                  10      //User Interrupt
#define SYSTICK_IRQ_PRIORITY                13      //CORTEX_M3 Systick Interrupt
#define SVCALL_IRQ_PRIORITY                 14      //CORTEX_M3 SVCall Interrupt
#define PENDSV_IRQ_PRIORITY                 15      //CORTEX_M3 PendSV Interrupt

#define PREPSTRING2(x) #x
#define PREPSTRING(x) PREPSTRING2(x)

#if PLATFORM_ID == PLATFORM_ANT || PLATFORM_ID == PLATFORM_L6 || PLATFORM_ID == PLATFORM_GPRS
#define INTERNAL_FLASH_SIZE                 (0x20000)
#else
    #pragma message "PLATFORM_ID is " PREPSTRING(PLATFORM_ID)
    #error "Unknown PLATFORM_ID"
#endif

/* Exported functions ------------------------------------------------------- */

#endif /* __PLATFORM_CONFIG_H */
