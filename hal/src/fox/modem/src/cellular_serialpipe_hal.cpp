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

#include "cellular_serialpipe_hal.h"
#include "hw_config.h"
#include "usart_hal.h"
#include "pinmap_impl.h"
#include "gpio_hal.h"
#include "mdm_hal.h"

#ifdef putc
#undef putc
#undef getc
#endif


UART_HandleTypeDef UartHandleCellular;

CellularSerialPipe::CellularSerialPipe(int rxSize, int txSize) :
    _pipeRx( rxSize ),
    _pipeTx( txSize )
{
    HAL_NVIC_DisableIRQ(USART2_IRQn);
}

CellularSerialPipe::~CellularSerialPipe(void)
{
    // wait for transmission of outgoing data
    __HAL_RCC_USART2_FORCE_RESET();
    __HAL_RCC_USART2_RELEASE_RESET();

    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2);
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_3);

    HAL_NVIC_DisableIRQ(USART2_IRQn);
}

void CellularSerialPipe::begin(unsigned int baud)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();

    GPIO_InitTypeDef  GPIO_InitStruct;
    /* UART TX GPIO pin configuration  */
    GPIO_InitStruct.Pin       = GPIO_PIN_2;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FAST;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    /* UART RX GPIO pin configuration  */
    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    UartHandleCellular.Instance          = USART2;
    UartHandleCellular.Init.BaudRate     = baud;
    UartHandleCellular.Init.WordLength   = UART_WORDLENGTH_8B;
    UartHandleCellular.Init.StopBits     = UART_STOPBITS_1;
    UartHandleCellular.Init.Parity       = UART_PARITY_NONE;
    UartHandleCellular.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    UartHandleCellular.Init.Mode         = UART_MODE_TX_RX;
    UartHandleCellular.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_DeInit(&UartHandleCellular);
    HAL_UART_Init(&UartHandleCellular);

    //Configure the NVIC for UART
    HAL_NVIC_SetPriority(USART2_IRQn, USART2_IRQ_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
    __HAL_UART_ENABLE_IT(&UartHandleCellular, UART_IT_RXNE);
}

// tx channel
int CellularSerialPipe::writeable(void)
{
    return 1;
}

int CellularSerialPipe::putc(int c)
{
    uint8_t data = c;
    HAL_UART_Transmit(&UartHandleCellular, &data, 1, 1000);//1s  带操作系统待验证
    return c;
}

int CellularSerialPipe::put(const void* buffer, int length, bool blocking)
{
    int n;
    const char* ptr = (const char*)buffer;

    for(n=0; n<length; n++)
    {
        putc(ptr[n]);
    }
    return length;
}

// rx channel
int CellularSerialPipe::readable(void)
{
    return _pipeRx.readable();
}

int CellularSerialPipe::getc(void)
{
    if (!_pipeRx.readable())
        return EOF;
    return _pipeRx.getc();
}

int CellularSerialPipe::get(void* buffer, int length, bool blocking)
{
    return _pipeRx.get((char*)buffer,length,blocking);
}

void CellularSerialPipe::rxIrqBuf(void)
{
    uint8_t c = (uint16_t)(UartHandleCellular.Instance->DR & (uint16_t)0x00FF);
    if (_pipeRx.writeable())
        _pipeRx.putc(c);
    else
        /* overflow */;
}

void CellularSerialPipe::rxResume(void)
{
}

void CellularSerialPipe::rxPause(void)
{
}

extern "C"
{
    void HAL_USART2_Handler(UART_HandleTypeDef *huart)
    {
        if((__HAL_UART_GET_FLAG(huart, UART_FLAG_RXNE) != RESET)
                && (__HAL_UART_GET_IT_SOURCE(huart, UART_IT_RXNE) != RESET))
        {
            CellularMDM.rxIrqBuf();
        }
    }

    // Serial1 interrupt handler
    void USART2_IRQHandler(void)
    {
        HAL_USART2_Handler(&UartHandleCellular);
    }
}
