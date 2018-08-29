/**
 ******************************************************************************
 * @file    adc_hal.c
 * @authors Matthew McGowan
 * @version V1.0.0
 * @date    27-Sept-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-2015 IntoRobot Industries, Inc.  All rights reserved.

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

#include "adc_hal.h"
#include "gpio_hal.h"
#include "pinmap_hal.h"
#include "pinmap_impl.h"
#include "molmc_log.h"

#define ADC_DMA_BUFFERSIZE    10
#define ADC_SAMPLING_TIME     ADC_SAMPLETIME_41CYCLES_5

/* Private variables ---------------------------------------------------------*/
// FIXME:Here not use volatile, it will occur volatile pointer error in C, but it is OK in C++
uint16_t ADC_ConvertedValues[ADC_DMA_BUFFERSIZE];
uint8_t adcInitFirstTime = true;
uint8_t adcChannelConfigured = ADC_CHANNEL_NONE;
static uint8_t ADC_Sample_Time = ADC_SAMPLING_TIME;
ADC_HandleTypeDef ADC_HandleStruct;
DMA_HandleTypeDef DMA_HandleStruct;


void HAL_ADC_Set_Sample_Time(uint8_t ADC_SampleTime)
{
    if(ADC_SampleTime < ADC_SAMPLETIME_1CYCLE_5 || ADC_SampleTime > ADC_SAMPLETIME_239CYCLES_5)
        {
            ADC_Sample_Time = ADC_SAMPLING_TIME;
        }
    else
        {
            ADC_Sample_Time = ADC_SampleTime;
    }
}

/*
 * @brief Read the analog value of a pin.
 * Should return a 16-bit value, 0-65536 (0 = LOW, 65536 = HIGH)
 * Note: ADC is 12-bit. Currently it returns 0-4096
 */
int32_t HAL_ADC_Read(uint16_t pin)
{
    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();

    if (PIN_MAP[pin].pin_mode != AN_INPUT)
    {
        HAL_GPIO_Save_Pin_Mode(pin);
        HAL_Pin_Mode(pin, AN_INPUT);
    }

    if (adcInitFirstTime == true)
    {
        RCC_PeriphCLKInitTypeDef  PeriphClkInit;
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
        PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
        HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);

        HAL_ADC_DMA_Init();
        adcInitFirstTime = false;
    }

    if (adcChannelConfigured != PIN_MAP[pin].adc_channel)
    {
        adcChannelConfigured = PIN_MAP[pin].adc_channel;

        __HAL_RCC_ADC1_CLK_ENABLE();
        /* Configure the ADC peripheral */
        ADC_HandleStruct.Instance                   = ADC1;
        ADC_HandleStruct.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
        ADC_HandleStruct.Init.ScanConvMode          = ADC_SCAN_DISABLE;
        ADC_HandleStruct.Init.ContinuousConvMode    = ENABLE;
        ADC_HandleStruct.Init.NbrOfConversion       = 1;
        ADC_HandleStruct.Init.DiscontinuousConvMode = DISABLE;
        ADC_HandleStruct.Init.NbrOfDiscConversion   = 0;
        ADC_HandleStruct.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
        /* ADC_HandleStruct.Init.ExternalTrigConv      = ADC_EXTERNALTRIGCONVEDGE_NONE; */
        /* ADC_HandleStruct.Init.DMAContinuousRequests = ENABLE; */

        HAL_ADC_Init(&ADC_HandleStruct);

        /* Configure ADC regular channel */
        ADC_ChannelConfTypeDef ADC_ChannelConfStruct;

        ADC_ChannelConfStruct.Channel      = PIN_MAP[pin].adc_channel;
        ADC_ChannelConfStruct.Rank         = 1;
        ADC_ChannelConfStruct.SamplingTime = ADC_Sample_Time;
       // ADC_ChannelConfStruct.Offset       = 0;

        HAL_ADC_ConfigChannel(&ADC_HandleStruct, &ADC_ChannelConfStruct);
    }

    memset(ADC_ConvertedValues, 0, sizeof(ADC_ConvertedValues));

    /* Start the conversion process */
    HAL_ADC_Start_DMA(&ADC_HandleStruct, (uint32_t*)ADC_ConvertedValues, ADC_DMA_BUFFERSIZE);
    // FIXME: if not DEBUG line below, it will not work right.
    HAL_Delay(1);
    HAL_ADC_Stop_DMA(&ADC_HandleStruct);
    HAL_ADC_Stop(&ADC_HandleStruct);

    uint32_t ADC_SummatedValue = 0;
    uint16_t ADC_AveragedValue = 0;

    int i = 0;
    for(i = 0; i < ADC_DMA_BUFFERSIZE; i++)
    {
        ADC_SummatedValue += ADC_ConvertedValues[i];
    }
    ADC_AveragedValue = (uint16_t)(ADC_SummatedValue / ADC_DMA_BUFFERSIZE);

    // Return ADC averaged value
    return ADC_AveragedValue;
}

/*
 * @brief Initialize the ADC peripheral.
 */
void HAL_ADC_DMA_Init()
{
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* Configure the DMA streams */
    DMA_HandleStruct.Instance                 = DMA1_Channel1;

    DMA_HandleStruct.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    DMA_HandleStruct.Init.PeriphInc           = DMA_PINC_DISABLE;
    DMA_HandleStruct.Init.MemInc              = DMA_MINC_ENABLE;
    DMA_HandleStruct.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    DMA_HandleStruct.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
    DMA_HandleStruct.Init.Mode                = DMA_CIRCULAR;
    DMA_HandleStruct.Init.Priority            = DMA_PRIORITY_LOW;
    HAL_DMA_Init(&DMA_HandleStruct);

    /* Associate the initialized DMA handle to the ADC handle */
    __HAL_LINKDMA(&ADC_HandleStruct, DMA_Handle, DMA_HandleStruct);

    // NOTE: Don't configure the priority of NVIC here
    /* Configure the NVIC for DMA */
    //HAL_NVIC_SetPriority(DMA2_Stream4_IRQn, 6, 0);
    //HAL_NVIC_EnableIRQ(DMA2_Stream4_IRQn);
}

/*
 * @brief Initialize the ADC peripheral.
 */
void DMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(ADC_HandleStruct.DMA_Handle);
}
