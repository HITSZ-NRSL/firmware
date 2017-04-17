/*
 * File	: uart.h
 * Copyright (C) 2013 - 2016, Espressif Systems
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif


void usart_cellular_initial(uint32_t baud);
void Cellular_GPIO_Initial(void);
void Cellular_Power_On(void);
void Cellular_Enter_UpdateMode(void);
void HAL_System_Config(void);
system_tick_t millis(void);
void delay(uint32_t ms);
void System_Reset(void);
void System_LineCodingBitRateHandler(uint32_t bitrate);

#ifdef __cplusplus
}
#endif



#endif

