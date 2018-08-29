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
#include "i2s_hal.h"

const static char *TAG = "hal";

#define TOTAL_I2S 2
// I2Snum_SD_CK
typedef enum I2S_Num_Def {
    I2S1_D3_D0_USER = 0,
    I2S2_CK_SD_SENSOR = 1
} I2S_Num_Def;

typedef struct STM32_I2S_Info {
    SPI_TypeDef* I2S_Peripheral;

    uint32_t I2S_DMA_Channel;

    //DMA_Stream_TypeDef* I2S_TX_DMA_Stream;
    DMA_Stream_TypeDef* I2S_RX_DMA_Stream;

    //IRQn_Type I2S_DMA_TX_IRQn;
    IRQn_Type I2S_DMA_RX_IRQn;

    GPIO_TypeDef* I2S_SK_Port;
    GPIO_TypeDef* I2S_SD_Port;

    uint16_t I2S_SK_Pin;
    uint16_t I2S_SD_Pin;
    uint8_t I2S_AF_Mapping;
    //DMA_HandleTypeDef hdma_tx;
    DMA_HandleTypeDef hdma_rx;
    I2S_HandleTypeDef I2SHandle;

    bool I2S_Enabled;
} STM32_I2S_Info;

/*
 * I2S mapping
 * I2S1: CK PB3(D3); SD PB5(D0)
 * I2S2: CK PB10;    SD PB15
 */
STM32_I2S_Info I2S_MAP[TOTAL_I2S] =
{
    { SPI3, DMA_CHANNEL_0, DMA1_Stream0, DMA1_Stream0_IRQn, GPIOB, GPIOB, GPIO_PIN_3, GPIO_PIN_5, GPIO_AF6_SPI3},
    { SPI2, DMA_CHANNEL_0, DMA1_Stream3, DMA1_Stream3_IRQn, GPIOB, GPIOB, GPIO_PIN_10, GPIO_PIN_15, GPIO_AF5_SPI2}
};

static STM32_I2S_Info *i2sMap[TOTAL_I2S];
I2S_HandleTypeDef *sensorI2SHandle = (I2S_HandleTypeDef *)&I2S_MAP[I2S2_CK_SD_SENSOR].I2SHandle;

/* Private typedef -----------------------------------------------------------*/

/*
 * @brief Initialize the i2s peripheral and DMA.
 * @param i2s: i2s number chosed
 * @retral None
 */
void HAL_I2S_GPIO_DMA_Init(HAL_I2S_Interface i2s)
{
    //MOLMC_LOGD(TAG, "Enter HAL_I2S_GPIO_DMA_Init...");

    /*##-1- Enable peripherals and GPIO Clocks #################################*/
    /* Enable GPIO SK  SD clock and DMA clock */
    if (i2sMap[i2s]->I2S_Peripheral == SPI3)
    {
        //MOLMC_LOGD(TAG, "Select I2S1, and Enable Clock...");
        __HAL_RCC_SPI3_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        // DMA2 clock
        __HAL_RCC_DMA1_CLK_ENABLE();
    }
    else if(i2sMap[i2s]->I2S_Peripheral == SPI2)
    {
        //MOLMC_LOGD(TAG, "Select I2S2, and Enable Clock...");
        __HAL_RCC_SPI2_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        // DMA1 clock
        __HAL_RCC_DMA1_CLK_ENABLE();
    }
    GPIO_InitTypeDef  GPIO_InitStruct;
    /*##-2- Configure peripheral GPIO ##########################################*/
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FAST;

    /* I2S SCK GPIO pin configuration  */
    GPIO_InitStruct.Pin       = i2sMap[i2s]->I2S_SK_Pin;
    GPIO_InitStruct.Alternate = i2sMap[i2s]->I2S_AF_Mapping;
    HAL_GPIO_Init( i2sMap[i2s]->I2S_SK_Port, &GPIO_InitStruct);

    /* I2S SD GPIO pin configuration  */
    GPIO_InitStruct.Pin = i2sMap[i2s]->I2S_SD_Pin;
    GPIO_InitStruct.Alternate = i2sMap[i2s]->I2S_AF_Mapping;
    HAL_GPIO_Init( i2sMap[i2s]->I2S_SD_Port, &GPIO_InitStruct);

    /*##-3- Configure the DMA streams ##########################################*/
    /* Configure the DMA handler for Transmission process */
    i2sMap[i2s]->hdma_rx.Instance                 = i2sMap[i2s]->I2S_RX_DMA_Stream;

    i2sMap[i2s]->hdma_rx.Init.Channel             = i2sMap[i2s]->I2S_DMA_Channel;
    i2sMap[i2s]->hdma_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    i2sMap[i2s]->hdma_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
    i2sMap[i2s]->hdma_rx.Init.MemInc              = DMA_MINC_ENABLE;
    i2sMap[i2s]->hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    i2sMap[i2s]->hdma_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
    if(i2sMap[i2s]->I2S_Peripheral == SPI3) // for user
    {
        i2sMap[i2s]->hdma_rx.Init.Mode            = DMA_NORMAL;//DMA_NORMAL; DMA_CIRCULAR
    }
    else if(i2sMap[i2s]->I2S_Peripheral == SPI2) // for sensor
    {
        i2sMap[i2s]->hdma_rx.Init.Mode            = DMA_CIRCULAR;//DMA_NORMAL; DMA_CIRCULAR
    }
    i2sMap[i2s]->hdma_rx.Init.Priority            = DMA_PRIORITY_HIGH;
    i2sMap[i2s]->hdma_rx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    i2sMap[i2s]->hdma_rx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    i2sMap[i2s]->hdma_rx.Init.MemBurst            = DMA_MBURST_SINGLE;
    i2sMap[i2s]->hdma_rx.Init.PeriphBurst         = DMA_PBURST_SINGLE;

    /* Associate the initialized DMA handle to the the I2S handle */
    __HAL_LINKDMA(&i2sMap[i2s]->I2SHandle, hdmarx, i2sMap[i2s]->hdma_rx);

    /* Deinitialize the Stream for new transfer */
    HAL_DMA_DeInit(&i2sMap[i2s]->hdma_rx);

    /* Configure the DMA Stream */
    HAL_DMA_Init(&i2sMap[i2s]->hdma_rx);

    /*##-4- Configure the NVIC for DMA #########################################*/
    /* NVIC configuration for DMA transfer complete interrupt (I2S_RX) */
    //HAL_NVIC_SetPriority(i2sMap[i2s]->I2S_DMA_RX_IRQn, 0x06, 0);
    //HAL_NVIC_EnableIRQ(i2sMap[i2s]->I2S_DMA_RX_IRQn);
    HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 0x08, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);

    //MOLMC_LOGD(TAG, "Leave HAL_I2S_GPIO_DMA_Init...");
}

/*
 * @brief De-Initialize the I2S peripheral and DMA.
 * @param i2s: i2s number chosed
 * @retral None
 */
void HAL_I2S_GPIO_DMA_DeInit(HAL_I2S_Interface i2s)
{
    //MOLMC_LOGD(TAG, "Enter HAL_I2S_GPIO_DMA_DeInit...");

    /*##-1- Reset peripherals ##################################################*/
    if (i2sMap[i2s]->I2S_Peripheral == SPI3)
    {
        __HAL_RCC_SPI3_FORCE_RESET();
        __HAL_RCC_SPI3_RELEASE_RESET();
    }
    else if(i2sMap[i2s]->I2S_Peripheral == SPI2)
    {
        __HAL_RCC_SPI2_FORCE_RESET();
        __HAL_RCC_SPI2_RELEASE_RESET();
    }

    /*##-2- Disable peripherals and GPIO Clocks ################################*/
    /* Configure I2S SCK as alternate function  */
    HAL_GPIO_DeInit(i2sMap[i2s]->I2S_SK_Port, i2sMap[i2s]->I2S_SK_Pin);
    /* Configure I2S SD as alternate function  */
    HAL_GPIO_DeInit(i2sMap[i2s]->I2S_SD_Port, i2sMap[i2s]->I2S_SD_Pin);

    /*##-3- Disable the DMA Streams ############################################*/
    /* De-Initialize the DMA Stream associate to reception process */
    HAL_DMA_DeInit(&i2sMap[i2s]->hdma_rx);

    /*##-4- Disable the NVIC for DMA ###########################################*/
    HAL_NVIC_DisableIRQ(i2sMap[i2s]->I2S_DMA_RX_IRQn);
}

/*
 * @brief Initial the I2S, include setup which I2S used, and some flags.
 * @param i2s: i2s number chosed
 * @retral None
 */
void HAL_I2S_Initial(HAL_I2S_Interface i2s)
{
    if(i2s == HAL_I2S_INTERFACE1)
    {
        //MOLMC_LOGD(TAG, "HAL_I2S_Initial Select I2S1...");
        i2sMap[i2s] = &I2S_MAP[I2S1_D3_D0_USER];
    }
    else if(i2s == HAL_I2S_INTERFACE2)
    {
        //MOLMC_LOGD(TAG, "HAL_I2S_Intial Select I2S2...");
        i2sMap[i2s] = &I2S_MAP[I2S2_CK_SD_SENSOR];
    }

    //MOLMC_LOGD(TAG, "Enter HAL_I2S_Initial...");
    i2sMap[i2s]->I2S_Enabled = false;
}

/*
 * @brief Begin the I2S
 * @param i2s: i2s number chosed
 * @param pin: NSS pin, not used
 * @retral None
 */
void HAL_I2S_Begin(HAL_I2S_Interface i2s, uint32_t audioFreq)
{
    //MOLMC_LOGD(TAG, "Enter HAL_I2S_Begin...");

    /* I2S configuration */
    i2sMap[i2s]->I2SHandle.Instance            = i2sMap[i2s]->I2S_Peripheral;

    __HAL_I2S_DISABLE(&i2sMap[i2s]->I2SHandle);

    /* I2S2 peripheral configuration */
    i2sMap[i2s]->I2SHandle.Init.AudioFreq   = 2 * audioFreq;
    i2sMap[i2s]->I2SHandle.Init.ClockSource = I2S_CLOCK_PLL;
    i2sMap[i2s]->I2SHandle.Init.CPOL        = I2S_CPOL_HIGH;
    i2sMap[i2s]->I2SHandle.Init.DataFormat  = I2S_DATAFORMAT_16B;
    i2sMap[i2s]->I2SHandle.Init.MCLKOutput  = I2S_MCLKOUTPUT_DISABLE;
    i2sMap[i2s]->I2SHandle.Init.Mode        = I2S_MODE_MASTER_RX;
    i2sMap[i2s]->I2SHandle.Init.Standard    = I2S_STANDARD_LSB;

    /* Initialize the I2S peripheral with the structure above */
    if(HAL_I2S_GetState(&i2sMap[i2s]->I2SHandle) == HAL_I2S_STATE_RESET)
    {
        HAL_I2S_GPIO_DMA_Init(i2s);
    }

    HAL_I2S_Init(&i2sMap[i2s]->I2SHandle);
    i2sMap[i2s]->I2S_Enabled = true;

    //MOLMC_LOGD(TAG, "Leave HAL_I2S_Begin...");
}

/*
 * @brief End the I2S.
 * @param i2s: i2s number chosed
 * @retral None
 */
void HAL_I2S_End(HAL_I2S_Interface i2s)
{
    //MOLMC_LOGD(TAG, "Enter HAL_I2S_End...");
    HAL_I2S_GPIO_DMA_DeInit(i2s);
    i2sMap[i2s]->I2S_Enabled = false;
}

/*
 * @brief I2S send and recieve data.
 * @param i2s: i2s number chosed.
 * @param data: The data to be transmitted.
 * @retral The received data.
 */
uint16_t HAL_I2S_DMA_Receive_Data(HAL_I2S_Interface i2s)
{
    uint16_t data;
    HAL_I2S_Receive_DMA(&i2sMap[i2s]->I2SHandle, &data, 1);
    return data;
}

/*
 * @brief Check whether I2S is enabled or not.
 * @param i2s: i2s number chosed.
 * @retral The enabled flag: true, i2s have begin; false, the i2s not working.
 */
bool HAL_I2S_Is_Enabled(HAL_I2S_Interface i2s)
{
    //MOLMC_LOGD(TAG, "Enter HAL_I2S_Is_Enabled...");
    return i2sMap[i2s]->I2S_Enabled;
}

/**
 * @brief  This function handles I2S IT Stream interrupt request.
 * @param  None
 * @retval None
 */
void DMA1_Stream0_IRQHandler(void)
{
    if(i2sMap[I2S1_D3_D0_USER]->I2S_Enabled == true)
    {
        HAL_DMA_IRQHandler(i2sMap[I2S1_D3_D0_USER]->I2SHandle.hdmarx);
    }
}

/**
 * @brief  This function handles I2S IT Stream interrupt request.
 * @param  None
 * @retval None
 */
void DMA1_Stream3_IRQHandler(void)
{
    if(i2sMap[I2S2_CK_SD_SENSOR]->I2S_Enabled == true)
    {
        HAL_DMA_IRQHandler(i2sMap[I2S2_CK_SD_SENSOR]->I2SHandle.hdmarx);
    }
}
