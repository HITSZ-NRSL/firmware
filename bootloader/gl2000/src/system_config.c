#include "hw_config.h"
#include "rtc_hal.h"
#include "ui_hal.h"
#include "boot_debug.h"


#define ESP8266_EN_GPIO_PIN             GPIO_PIN_9
#define ESP8266_EN_GPIO_PORT            GPIOA
#define ESP8266_RST_GPIO_PIN            GPIO_PIN_10
#define ESP8266_RST_GPIO_PORT           GPIOA
#define SIM800C_PWR_EN_GPIO_PIN         GPIO_PIN_8
#define SIM800C_PWR_EN_GPIO_PORT        GPIOB

#define ESP8266_USART_QUEUE_SIZE              1024

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
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    GPIO_InitTypeDef  GPIO_InitStruct;
    /* UART TX GPIO pin configuration  */
    GPIO_InitStruct.Pin       = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FAST;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    UartHandleDebug.Instance          = USART1;
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
    HAL_NVIC_DisableIRQ(USART2_IRQn);
    sdkReleaseQueue(&USART_Esp8266_Queue);

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

    UartHandleEsp8266.Instance          = USART2;
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
    HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
    __HAL_UART_ENABLE_IT(&UartHandleEsp8266, UART_IT_RXNE);
}

void HAL_USART2_Esp8266_Handler(UART_HandleTypeDef *huart)
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

    //ESP8266_EN
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Pin = ESP8266_EN_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(ESP8266_EN_GPIO_PORT, &GPIO_InitStruct);
    //ESP8266_RST
    GPIO_InitStruct.Pin = ESP8266_RST_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(ESP8266_RST_GPIO_PORT, &GPIO_InitStruct);
    //SIM800C PWR_EN 关闭SIM800C电源
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitStruct.Pin = SIM800C_PWR_EN_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(SIM800C_PWR_EN_GPIO_PORT, &GPIO_InitStruct);
    //SIM800C 暂停工作
    HAL_GPIO_WritePin(SIM800C_PWR_EN_GPIO_PORT, SIM800C_PWR_EN_GPIO_PIN, GPIO_PIN_RESET);
}

void Esp8266_Reset(void)
{
    HAL_GPIO_WritePin(ESP8266_EN_GPIO_PORT, ESP8266_EN_GPIO_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(ESP8266_RST_GPIO_PORT, ESP8266_RST_GPIO_PIN, GPIO_PIN_RESET);
    HAL_Delay(200);
    HAL_GPIO_WritePin(ESP8266_RST_GPIO_PORT, ESP8266_RST_GPIO_PIN, GPIO_PIN_SET);
}

void HAL_System_Config(void)
{
    Set_System();
#if defined(DEBUG_BUILD)
    usart_debug_initial(115200);
#endif
    HAL_UI_Initial();
    HAL_UI_UserLED_Control(0);
    HAL_RTC_Initial();
    usart_esp8266_initial(115200);  //esp8266通讯 采取115200波特率
    ESP8266_GPIO_Initial();
    Esp8266_Reset();
}

void Esp8266_Enter_UpdateMode(void)
{
    GPIO_InitTypeDef  GPIO_InitStruct;

    HAL_NVIC_DisableIRQ(USART2_IRQn);
    sdkReleaseQueue(&USART_Esp8266_Queue);

    //ESP8266_USART
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Pin       = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode      = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    //ESP8266_RST
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Pin       = ESP8266_RST_GPIO_PIN;
    GPIO_InitStruct.Mode      = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(ESP8266_RST_GPIO_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(ESP8266_EN_GPIO_PORT, ESP8266_EN_GPIO_PIN, GPIO_PIN_SET);
}

system_tick_t millis(void)
{
    return HAL_Tick_Get_Milli_Seconds();
}

unsigned long micros(void)
{
    return HAL_Tick_Get_Micro_Seconds();
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

