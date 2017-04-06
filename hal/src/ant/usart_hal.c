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

/* Includes ------------------------------------------------------------------*/
#include "hw_config.h"
#include "usart_hal.h"
#include "pinmap_impl.h"
#include <string.h>

#define SERIAL_QUEUE_SIZE  256

UART_HandleTypeDef UartHandle_SERIAL1;    // USART1 (PA10)-RX (PA9)-TX
UART_HandleTypeDef UartHandle_SERIAL2;    // USART2 (PA3)-RX  (PA2)-TX
UART_HandleTypeDef UartHandle_SERIAL3;    // USART3 (PB11)-RX (PB10)-TX

SDK_QUEUE Usart_Rx_Queue_SERIAL1;
SDK_QUEUE Usart_Rx_Queue_SERIAL2;
SDK_QUEUE Usart_Rx_Queue_SERIAL3;

/* Private typedef -----------------------------------------------------------*/
typedef enum USART_Num_Def {
    USART_SERIAL1 = 0,
    USART_SERIAL2 = 1,
    USART_SERIAL3 = 2,
}USART_Num_Def;

#define IS_USART_CONFIG_VALID(CONFIG) (((CONFIG & SERIAL_VALID_CONFIG) >> 2) != 0b11)


/* Private variables ---------------------------------------------------------*/
typedef struct STM32_USART_Info {
    USART_TypeDef* usart_peripheral;
    uint32_t usart_alternate;
    int32_t usart_int_n;
    uint16_t usart_tx_pin;
    uint16_t usart_rx_pin;

    UART_HandleTypeDef *uart_handle;
    // Buffer pointers. These need to be global for IRQ handler access
    SDK_QUEUE *usart_tx_queue;
    SDK_QUEUE *usart_rx_queue;

    bool usart_enabled;
    bool usart_transmitting;
} STM32_USART_Info;

/*
 * USART mapping
 */
STM32_USART_Info USART_MAP[TOTAL_USARTS] =
{
    /*
     * USART_peripheral (USARTx/UARTx; not using others)
     * gpio_peripheral (GPIOA, GPIOB, GPIOC or GPIOD)
     * Alternate;
     * interrupt number (USARTx_IRQn/UARTx_IRQn)
     * TX pin
     * RX pin
     * <tx_buffer pointer> used internally and does not appear below
     * <rx_buffer pointer> used internally and does not appear below
     * <usart enabled> used internally and does not appear below
     * <usart transmitting> used internally and does not appear below
     */

    { USART1, GPIO_AF7_USART1, USART1_IRQn, TX, RX },    // USART1
    { USART2, GPIO_AF7_USART2, USART2_IRQn, TX1, RX1 },  // USART2
    { USART3, GPIO_AF7_USART3, USART3_IRQn, TX2, RX2 },  // USART3
};

static STM32_USART_Info *usartMap[TOTAL_USARTS]; // pointer to USART_MAP[] containing USART peripheral register locations (etc)

void HAL_USART_Initial(HAL_USART_Serial serial)
{
    if(serial == HAL_USART_SERIAL1)
    {
        usartMap[serial] = &USART_MAP[USART_SERIAL1];
        usartMap[serial]->uart_handle = &UartHandle_SERIAL1;
        usartMap[serial]->usart_rx_queue = &Usart_Rx_Queue_SERIAL1;
        sdkInitialQueue(usartMap[serial]->usart_rx_queue, SERIAL_QUEUE_SIZE);
    }
    else if(serial == HAL_USART_SERIAL2)
    {
        usartMap[serial] = &USART_MAP[USART_SERIAL2];
        usartMap[serial]->uart_handle = &UartHandle_SERIAL2;
        usartMap[serial]->usart_rx_queue = &Usart_Rx_Queue_SERIAL2;
        sdkInitialQueue(usartMap[serial]->usart_rx_queue, SERIAL_QUEUE_SIZE);
    }
    else if(serial == HAL_USART_SERIAL3)
    {
        usartMap[serial] = &USART_MAP[USART_SERIAL3];
        usartMap[serial]->uart_handle = &UartHandle_SERIAL3;
        usartMap[serial]->usart_rx_queue = &Usart_Rx_Queue_SERIAL3;
        sdkInitialQueue(usartMap[serial]->usart_rx_queue, SERIAL_QUEUE_SIZE);
    }

    usartMap[serial]->usart_enabled = false;
    usartMap[serial]->usart_transmitting = false;
}

void HAL_USART_Begin(HAL_USART_Serial serial, uint32_t baud)
{
    HAL_USART_BeginConfig(serial, baud, 0, 0); // Default serial configuration is 8N1
}

void HAL_USART_BeginConfig(HAL_USART_Serial serial, uint32_t baud, uint32_t config, void *ptr)
{
    if( true == usartMap[serial]->usart_enabled )
    {
        return;
    }

    // Verify UART configuration, exit if it's invalid.
    if (!IS_USART_CONFIG_VALID(config))
    {
        usartMap[serial]->usart_enabled = false;
        return;
    }

    if(HAL_USART_SERIAL1 == serial)
    {
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_USART1_CLK_ENABLE();

    }
    else if(HAL_USART_SERIAL2 == serial)
    {
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_USART2_CLK_ENABLE();

    }
    else if(HAL_USART_SERIAL3 == serial)
    {
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_USART3_CLK_ENABLE();

    }

    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    GPIO_InitTypeDef  GPIO_InitStruct;
    /* UART TX RX GPIO pin configuration  */
    GPIO_InitStruct.Pin       = PIN_MAP[usartMap[serial]->usart_tx_pin].gpio_pin|PIN_MAP[usartMap[serial]->usart_rx_pin].gpio_pin;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;//GPIO_SPEED_FAST;
    GPIO_InitStruct.Alternate = usartMap[serial]->usart_alternate;
    HAL_GPIO_Init(PIN_MAP[usartMap[serial]->usart_rx_pin].gpio_peripheral, &GPIO_InitStruct);

    usartMap[serial]->uart_handle->Instance          = usartMap[serial]->usart_peripheral;
    usartMap[serial]->uart_handle->Init.BaudRate     = baud;
    usartMap[serial]->uart_handle->Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    usartMap[serial]->uart_handle->Init.Mode         = UART_MODE_TX_RX;
    usartMap[serial]->uart_handle->Init.OverSampling = UART_OVERSAMPLING_16;

    // Stop bit configuration.
    switch (config & SERIAL_STOP_BITS)
    {
        case 0b00: // 1 stop bit
            usartMap[serial]->uart_handle->Init.StopBits = UART_STOPBITS_1;
            break;
        case 0b01: // 2 stop bits
            usartMap[serial]->uart_handle->Init.StopBits = UART_STOPBITS_2;
            break;
    }

    // Eight / Nine data bit configuration
    if (config & SERIAL_NINE_BITS)
    {
        // Nine data bits, no parity.
        usartMap[serial]->uart_handle->Init.Parity = UART_PARITY_NONE;
        usartMap[serial]->uart_handle->Init.WordLength = UART_WORDLENGTH_9B;
    }
    else
    {
        // eight data bits, parity configuration (impacts word length)
        switch ((config & SERIAL_PARITY_BITS) >> 2)
        {
            case 0b00: // none
                usartMap[serial]->uart_handle->Init.Parity = UART_PARITY_NONE;
                usartMap[serial]->uart_handle->Init.WordLength = UART_WORDLENGTH_8B;
                break;
            case 0b01: // even
                usartMap[serial]->uart_handle->Init.Parity = UART_PARITY_EVEN;
                usartMap[serial]->uart_handle->Init.WordLength = UART_WORDLENGTH_9B;
                break;
            case 0b10: // odd
                usartMap[serial]->uart_handle->Init.Parity = UART_PARITY_ODD;
                usartMap[serial]->uart_handle->Init.WordLength = UART_WORDLENGTH_9B;
                break;
        }
    }

    HAL_UART_DeInit(usartMap[serial]->uart_handle);
    HAL_UART_Init(usartMap[serial]->uart_handle);

    //Configure the NVIC for UART
    HAL_NVIC_SetPriority(usartMap[serial]->usart_int_n, 0x07, 0);
    HAL_NVIC_EnableIRQ(usartMap[serial]->usart_int_n);
    __HAL_UART_ENABLE_IT(usartMap[serial]->uart_handle, UART_IT_RXNE);

    usartMap[serial]->usart_enabled = true;
    usartMap[serial]->usart_transmitting = false;
}

void HAL_USART_End(HAL_USART_Serial serial)
{
    HAL_UART_DeInit(usartMap[serial]->uart_handle);

    if(HAL_USART_SERIAL1 == serial)
    {
        __HAL_RCC_USART1_FORCE_RESET();
        __HAL_RCC_USART1_RELEASE_RESET();
    }
    else if(HAL_USART_SERIAL2 == serial)
    {
        __HAL_RCC_USART2_FORCE_RESET();
        __HAL_RCC_USART2_RELEASE_RESET();
    }
    else if(HAL_USART_SERIAL3 == serial)
    {
        __HAL_RCC_USART3_FORCE_RESET();
        __HAL_RCC_USART3_RELEASE_RESET();
    }

    //Disable the NVIC for UART ##########################################*/
    HAL_NVIC_DisableIRQ(usartMap[serial]->usart_int_n);

    // clear any received data
    sdkReleaseQueue(usartMap[serial]->usart_rx_queue);

    // Undo any pin re-mapping done for this USART
    // ...
	STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    /* Configure UART Tx as alternate function */
    HAL_GPIO_DeInit(PIN_MAP[usartMap[serial]->usart_tx_pin].gpio_peripheral, PIN_MAP[usartMap[serial]->usart_tx_pin].gpio_pin);
    /* Configure UART Rx as alternate function */
    HAL_GPIO_DeInit(PIN_MAP[usartMap[serial]->usart_tx_pin].gpio_peripheral, PIN_MAP[usartMap[serial]->usart_tx_pin].gpio_pin);

    usartMap[serial]->usart_enabled = false;
    usartMap[serial]->usart_transmitting = false;
}

uint32_t HAL_USART_Write_Data(HAL_USART_Serial serial, uint8_t data)
{
    HAL_UART_Transmit(usartMap[serial]->uart_handle, (uint8_t *)&data, 1, 100);//100ms  带操作系统待验证
    return 1;
}

uint32_t HAL_USART_Write_NineBitData(HAL_USART_Serial serial, uint16_t data)
{
    return 1;
}

int32_t HAL_USART_Available_Data(HAL_USART_Serial serial)
{
    return sdkGetQueueDataLen(usartMap[serial]->usart_rx_queue);
}

int32_t HAL_USART_Available_Data_For_Write(HAL_USART_Serial serial)
{
    return 1;
}

int32_t HAL_USART_Read_Data(HAL_USART_Serial serial)
{
    uint8_t data;

    if(HAL_USART_Available_Data(serial))
    {
        sdkGetQueueData(usartMap[serial]->usart_rx_queue, &data);
        return data;
    }
    return -1;
}

int32_t HAL_USART_Peek_Data(HAL_USART_Serial serial)
{
     uint8_t data;

    if(HAL_USART_Available_Data(serial))
    {
        sdkTryQueueData(usartMap[serial]->usart_rx_queue, sdkGetQueueHead(usartMap[serial]->usart_rx_queue) , &data);
        return data;
    }
    return -1;
}

void HAL_USART_Flush_Data(HAL_USART_Serial serial)
{
    usartMap[serial]->usart_transmitting = false;
}

bool HAL_USART_Is_Enabled(HAL_USART_Serial serial)
{
    return usartMap[serial]->usart_enabled;
}

void HAL_USART_Half_Duplex(HAL_USART_Serial serial, bool Enable)
{

}

static void HAL_USART_Handler(HAL_USART_Serial serial)
{
    if((__HAL_UART_GET_FLAG(usartMap[serial]->uart_handle, UART_FLAG_RXNE) != RESET) && (__HAL_UART_GET_IT_SOURCE(usartMap[serial]->uart_handle, UART_IT_RXNE) != RESET))
    {
        // Read byte from the receive data register
        uint8_t c = (uint16_t)(usartMap[serial]->uart_handle->Instance->DR & (uint16_t)0x00FF);
        sdkInsertQueue(usartMap[serial]->usart_rx_queue, &c, 1);
    }
}
// Serial1 interrupt handler
void USART1_IRQHandler(void)
{
    HAL_USART_Handler(HAL_USART_SERIAL1);
}
// Serial2 interrupt handler
void USART2_IRQHandler(void)
{
    HAL_USART_Handler(HAL_USART_SERIAL2);
}
// Serial3 interrupt handler
void USART3_IRQHandler(void)
{
    HAL_USART_Handler(HAL_USART_SERIAL3);
}
