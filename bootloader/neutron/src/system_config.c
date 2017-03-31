#include "hw_config.h"
#include "rtc_hal.h"
#include "ui_hal.h"
#include "eeprom_hal.h"
#include "boot_debug.h"

#define ESP8266_USART_QUEUE_SIZE              (1024*20)

UART_HandleTypeDef UartHandleDebug;
UART_HandleTypeDef UartHandleEsp8266;

SDK_QUEUE USART_Esp8266_Queue;


void log_output(const char* msg)
{
    while(*msg)
    {
        HAL_UART_Transmit(&UartHandleDebug, (uint8_t *)msg++, 1, 10);
    }
}

void usart_debug_initial(uint32_t baud)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();

    GPIO_InitTypeDef  GPIO_InitStruct;
    /* UART TX GPIO pin configuration  */
    GPIO_InitStruct.Pin       = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FAST;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    UartHandleDebug.Instance          = USART2;
    UartHandleDebug.Init.BaudRate     = baud;
    UartHandleDebug.Init.WordLength   = UART_WORDLENGTH_8B;
    UartHandleDebug.Init.StopBits     = UART_STOPBITS_1;
    UartHandleDebug.Init.Parity       = UART_PARITY_NONE;
    UartHandleDebug.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    UartHandleDebug.Init.Mode         = UART_MODE_TX_RX;
    UartHandleDebug.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_DeInit(&UartHandleDebug);
    HAL_UART_Init(&UartHandleDebug);

    //set_logger_output(log_output, ALL_LEVEL); //注册debug实现函数
}

void usart_esp8266_initial(uint32_t baud)
{
    HAL_NVIC_DisableIRQ(USART1_IRQn);
    sdkReleaseQueue(&USART_Esp8266_Queue);

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    GPIO_InitTypeDef  GPIO_InitStruct;
    /* UART TX GPIO pin configuration  */
    GPIO_InitStruct.Pin       = GPIO_PIN_9 | GPIO_PIN_10;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FAST;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    UartHandleEsp8266.Instance          = USART1;
    UartHandleEsp8266.Init.BaudRate     = baud;
    UartHandleEsp8266.Init.WordLength   = UART_WORDLENGTH_8B;
    UartHandleEsp8266.Init.StopBits     = UART_STOPBITS_1;
    UartHandleEsp8266.Init.Parity       = UART_PARITY_NONE;
    UartHandleEsp8266.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    UartHandleEsp8266.Init.Mode         = UART_MODE_TX_RX;
    UartHandleEsp8266.Init.OverSampling = UART_OVERSAMPLING_16;

    HAL_UART_DeInit(&UartHandleEsp8266);
    HAL_UART_Init(&UartHandleEsp8266);

    sdkInitialQueue(&USART_Esp8266_Queue, ESP8266_USART_QUEUE_SIZE); //初始化esp8266接受缓冲队列
    //Configure the NVIC for UART
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
    __HAL_UART_ENABLE_IT(&UartHandleEsp8266, UART_IT_RXNE);
}

void HAL_USART1_Esp8266_Handler(UART_HandleTypeDef *huart)
{
    uint8_t c;

    if((__HAL_UART_GET_FLAG(huart, UART_FLAG_RXNE) != RESET)
            && (__HAL_UART_GET_IT_SOURCE(huart, UART_IT_RXNE) != RESET))
    {
        c = (uint16_t)(huart->Instance->DR & (uint16_t)0x00FF);
        sdkInsertQueue(&USART_Esp8266_Queue, &c, 1);
    }
}

void ESP8266_GPIO_Initial(void)
{
    GPIO_InitTypeDef   GPIO_InitStruct;
    //esp8266 gpio0
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    //esp8266 reset
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

void Esp8266_Reset(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
    HAL_Delay(200);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
}

void HAL_System_Config(void)
{
    Set_System();
    usart_debug_initial(115200);
    HAL_UI_Initial();
    HAL_UI_RGB_Color(RGB_COLOR_CYAN);
    HAL_RTC_Initial();
    usart_esp8266_initial(460800);  //esp8266通讯 采取460800波特率
    ESP8266_GPIO_Initial();
    Esp8266_Reset();
    HAL_EEPROM_Init();   //初始化eeprom区
}

void Esp8266_Enter_UpdateMode(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
    HAL_Delay(200);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
}

system_tick_t millis(void)
{
    return HAL_GetTick();
}

void delay(uint32_t ms)
{
    unsigned long startTime = millis();
    while( millis() - startTime < ms)
    {
    }
}

void System_Reset(void)
{
    NVIC_SystemReset();
}

void System_LineCodingBitRateHandler(uint32_t bitrate)
{
    usart_esp8266_initial(bitrate);
}

