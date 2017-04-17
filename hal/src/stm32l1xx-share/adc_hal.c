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

#include "hw_config.h"
#include "adc_hal.h"
#include "gpio_hal.h"
#include "pinmap_hal.h"
#include "pinmap_impl.h"

#define ADC_DMA_BUFFERSIZE    10
#define ADC_SAMPLING_TIME     ADC_SAMPLETIME_4CYCLES

/* Private variables ---------------------------------------------------------*/
// FIXME:Here not use volatile, it will occur volatile pointer error in C, but it is OK in C++
static uint16_t ADC_ConvertedValues[ADC_DMA_BUFFERSIZE];
static uint8_t adcInitFirstTime = true;
static uint8_t adcChannelConfigured = ADC_CHANNEL_NONE;
static uint8_t ADC_Sample_Time = ADC_SAMPLING_TIME;
static ADC_HandleTypeDef ADC_HandleStruct;
static DMA_HandleTypeDef DMA_HandleStruct;


/* Private function prototypes -----------------------------------------------*/
/*
 * @brief  Override the default ADC Sample time depending on requirement
 * @param  ADC_SampleTime: The sample time value to be set.
 * This parameter can be one of the following values:
 * @arg ADC_SAMPLETIME_4CYCLES:   Sampling time 4 ADC clock cycles
 * @arg ADC_SAMPLETIME_9CYCLES:
 * @arg ADC_SAMPLETIME_16CYCLES:
 * @arg ADC_SAMPLETIME_24CYCLES:
 * @arg ADC_SAMPLETIME_48CYCLES:
 * @arg ADC_SAMPLETIME_96CYCLES:
 * @arg ADC_SAMPLETIME_192CYCLES:
 * @arg ADC_SAMPLETIME_384CYCLES:
 * @retval None
 */
void HAL_ADC_Set_Sample_Time(uint8_t ADC_SampleTime)
{
    if(ADC_SampleTime < ADC_SAMPLETIME_4CYCLES || ADC_SampleTime > ADC_SAMPLETIME_384CYCLES)
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

    // FIXME: Not matter ADC DMA init only one time, or everytime init, it works.

    if(adcInitFirstTime)
    {
        __HAL_RCC_ADC1_CLK_ENABLE();
        HAL_ADC_DMA_Init();
        adcInitFirstTime = false;
    }

    if (adcChannelConfigured != PIN_MAP[pin].adc_channel)
    {
        adcChannelConfigured = PIN_MAP[pin].adc_channel;

        /* Configure the ADC peripheral */
        ADC_HandleStruct.Instance                   = ADC1;
        ADC_HandleStruct.Init.ClockPrescaler        = ADC_CLOCK_ASYNC_DIV4;
        ADC_HandleStruct.Init.Resolution            = ADC_RESOLUTION_12B;
        ADC_HandleStruct.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
        ADC_HandleStruct.Init.ScanConvMode          = ADC_SCAN_DISABLE;
        ADC_HandleStruct.Init.EOCSelection          = EOC_SINGLE_CONV;
        ADC_HandleStruct.Init.LowPowerAutoWait      = ADC_AUTOWAIT_DISABLE;
        ADC_HandleStruct.Init.LowPowerAutoPowerOff  = ADC_AUTOPOWEROFF_DISABLE;
        ADC_HandleStruct.Init.ChannelsBank          = ADC_CHANNELS_BANK_A;
        ADC_HandleStruct.Init.ContinuousConvMode    = ENABLE;
        ADC_HandleStruct.Init.NbrOfConversion       = 1;
        ADC_HandleStruct.Init.DiscontinuousConvMode = DISABLE;
        /* ADC_HandleStruct.Init.NbrOfDiscConversion   = 0; */
        ADC_HandleStruct.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
        ADC_HandleStruct.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
        ADC_HandleStruct.Init.DMAContinuousRequests = ENABLE;

        if(HAL_ADC_Init(&ADC_HandleStruct) != HAL_OK)
        {
            DEBUG("ADC Initialize Fial !!!");
        }

        /* Configure ADC regular channel */
        ADC_ChannelConfTypeDef ADC_ChannelConfStruct;

        ADC_ChannelConfStruct.Channel      = PIN_MAP[pin].adc_channel;
        ADC_ChannelConfStruct.Rank         = 1;
        ADC_ChannelConfStruct.SamplingTime = ADC_Sample_Time;

        if(HAL_ADC_ConfigChannel(&ADC_HandleStruct, &ADC_ChannelConfStruct) != HAL_OK)
        {
            DEBUG("ADC Config Channel Fail");
        }
    }

    memset(ADC_ConvertedValues, 0, sizeof(ADC_ConvertedValues));

    /* Start the conversion process */
    if( HAL_ADC_Start_DMA(&ADC_HandleStruct, (uint32_t*)ADC_ConvertedValues, ADC_DMA_BUFFERSIZE) != HAL_OK)
    {
        DEBUG("ADC DMA Transmission Fail !!!");
    }

    // FIXME: if not DEBUG line below, it will not work right.
    //必须加上延时 否则值不正确
    HAL_Delay(1);

    HAL_ADC_Stop_DMA(&ADC_HandleStruct);
    HAL_ADC_Stop(&ADC_HandleStruct);

    uint32_t ADC_SummatedValue = 0;
    uint16_t ADC_AveragedValue = 0;

    uint8_t i = 0;
    for(i = 0; i < ADC_DMA_BUFFERSIZE; i++)
    {
        ADC_SummatedValue += ADC_ConvertedValues[i];
    }
    ADC_AveragedValue = (uint16_t)(ADC_SummatedValue / ADC_DMA_BUFFERSIZE);

    return ADC_AveragedValue;
}

void HAL_ADC_DMA_Init(void)
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
    //不要开启DMA中断，由于ADC转换速度过快，系统会不停进入DMA中断中，所以不使能DMA中断
    /* Configure the NVIC for DMA */
    //HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 6, 0);
    //HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

}


#if 0
/*
 * @brief Initialize the ADC peripheral.
 */
void DMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(ADC_HandleStruct.DMA_Handle);
}
#endif
