/**
 ******************************************************************************
  Copyright (c) 2013-2014 IntoRobot Team.  All right reserved.

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
#ifndef __USART_HAL_H
#define __USART_HAL_H

#include <stdbool.h>

/* Includes ------------------------------------------------------------------*/
#include "pinmap_hal.h"

/* Exported defines ----------------------------------------------------------*/
#if PLATFORM_ID == 0 // atom
	#define TOTAL_USARTS		2
#else
	#define TOTAL_USARTS		1
#endif
#define SERIAL_BUFFER_SIZE      64

/* Exported types ------------------------------------------------------------*/
typedef struct Ring_Buffer
{
  unsigned char buffer[SERIAL_BUFFER_SIZE];
  volatile uint8_t head;
  volatile uint8_t tail;
} Ring_Buffer;

typedef enum HAL_USART_Serial {
  HAL_USART_SERIAL1 = 0,    //maps to USART_TX_RX
#if PLATFORM_ID == 10 // Electron
  HAL_USART_SERIAL2 = 1     //maps to USART_RGBG_RGBB
#endif
} HAL_USART_Serial;

/* Exported constants --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

void HAL_USART_Init(HAL_USART_Serial serial, Ring_Buffer *rx_buffer, Ring_Buffer *tx_buffer);
void HAL_USART_Begin(HAL_USART_Serial serial, uint32_t baud);
void HAL_USART_End(HAL_USART_Serial serial);
uint32_t HAL_USART_Write_Data(HAL_USART_Serial serial, uint8_t data);
int32_t HAL_USART_Available_Data_For_Write(HAL_USART_Serial serial);
int32_t HAL_USART_Available_Data(HAL_USART_Serial serial);
int32_t HAL_USART_Read_Data(HAL_USART_Serial serial);
int32_t HAL_USART_Peek_Data(HAL_USART_Serial serial);
void HAL_USART_Flush_Data(HAL_USART_Serial serial);
bool HAL_USART_Is_Enabled(HAL_USART_Serial serial);
void HAL_USART_Half_Duplex(HAL_USART_Serial serial, bool Enable);

#ifdef __cplusplus
}
#endif

#endif  /* __USART_HAL_H */
