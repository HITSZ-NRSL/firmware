/**
 ******************************************************************************
 * @file    tone_hal.c
 * @author  Satish Nair, Brett Walach
 * @version V1.0.0
 * @date    12-Sept-2014
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

/* Includes ------------------------------------------------------------------*/
#include "hw_config.h"
#include "tone_hal.h"
#include "pinmap_impl.h"


#define TONE_TIM_COUNTER_CLOCK_FREQ 1000000 ////TIM Counter clock = 1MHz

TIM_HandleTypeDef TimHandleTone;
/*
 * @brief Tone start with frequency and  duration
 * @param pin: The select pin
 * @param frequency: The frequency.
 * @param duration: The tone duration.
 * @retval None
 */
void HAL_Tone_Start(uint8_t pin, uint32_t frequency, uint32_t duration)
{
    if(frequency < 20 || frequency > 20000)
    {
        return;//no tone for frequency outside of human audible range
    }
    //Map the pin to the appropriate port and pin on the STM32
    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    if(PIN_MAP[pin].timer_peripheral != NULL)
    {
        /* Common configuration for all channles */
        GPIO_InitTypeDef GPIO_InitStruct;
        GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull  = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

        // else if( (PIN_MAP[pin].timer_peripheral == TIM2) )
        if( (PIN_MAP[pin].timer_peripheral == TIM2) )
        {
            __HAL_RCC_TIM2_CLK_ENABLE();

            GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
            GPIO_InitStruct.Pin       = PIN_MAP[pin].gpio_pin;
            /* Port Clock enable */
            if( (PIN_MAP[pin].gpio_peripheral == GPIOA) )
            {
                __HAL_RCC_GPIOA_CLK_ENABLE();
                HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
            }
            else if( (PIN_MAP[pin].gpio_peripheral == GPIOB) )
            {
                __HAL_RCC_GPIOB_CLK_ENABLE();
                HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
            }
            HAL_NVIC_SetPriority(TIM2_IRQn, 0x0E, 0);
            HAL_NVIC_EnableIRQ(TIM2_IRQn);
            //HAL_TIM2_Handler = Tone_TIM2_Handler;

        }
        else if( (PIN_MAP[pin].timer_peripheral == TIM3) )
        {
            // D0 and A7 share the same TIM3->CHANNEL2, only one can work at a time.
            // D2 and A6 share the same TIM3->CHANNEL1, only one can work at a time.
            __HAL_RCC_TIM3_CLK_ENABLE();
            GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
            GPIO_InitStruct.Pin       = PIN_MAP[pin].gpio_pin;

            /* Port Clock enable */
            if( (PIN_MAP[pin].gpio_peripheral == GPIOA) )
            {
                __HAL_RCC_GPIOA_CLK_ENABLE();
                HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
            }
            else if( (PIN_MAP[pin].gpio_peripheral == GPIOB) )
            {
                __HAL_RCC_GPIOB_CLK_ENABLE();
                HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
            }
            HAL_NVIC_SetPriority(TIM3_IRQn, 0x0E, 0);
            HAL_NVIC_EnableIRQ(TIM3_IRQn);

            //HAL_TIM3_Handler = Tone_TIM3_Handler;
        }
        else if( (PIN_MAP[pin].timer_peripheral == TIM9) )
        {
            __HAL_RCC_TIM9_CLK_ENABLE();
            GPIO_InitStruct.Alternate = GPIO_AF3_TIM9;
            GPIO_InitStruct.Pin       = PIN_MAP[pin].gpio_pin;

            /* Port Clock enable */
            if( (PIN_MAP[pin].gpio_peripheral == GPIOA) )
            {
                __HAL_RCC_GPIOA_CLK_ENABLE();
                HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
            }
            else if( (PIN_MAP[pin].gpio_peripheral == GPIOB) )
            {
                __HAL_RCC_GPIOB_CLK_ENABLE();
                HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
            }
            HAL_NVIC_SetPriority(TIM9_IRQn, 0x0E, 0);
            HAL_NVIC_EnableIRQ(TIM9_IRQn);
            //HAL_TIM4_Handler = Tone_TIM4_Handler;
        }
        else if( (PIN_MAP[pin].timer_peripheral == TIM11) )
        {
            __HAL_RCC_TIM11_CLK_ENABLE();

            GPIO_InitStruct.Alternate = GPIO_AF3_TIM11;
            GPIO_InitStruct.Pin       = PIN_MAP[pin].gpio_pin;

            /* Port Clock enable */
            if( (PIN_MAP[pin].gpio_peripheral == GPIOA) )
            {
                __HAL_RCC_GPIOA_CLK_ENABLE();
                HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
            }
            else if( (PIN_MAP[pin].gpio_peripheral == GPIOB) )
            {
                __HAL_RCC_GPIOB_CLK_ENABLE();
                HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
            }
            HAL_NVIC_SetPriority(TIM11_IRQn, 0x0E, 0);
            HAL_NVIC_EnableIRQ(TIM11_IRQn);
            //HAL_TIM5_Handler = Tone_TIM5_Handler;
        }
        // XXX: Note here SystemCoreClock Should be TIM CLK, here equal
        // TIM_CLK should be set in the above code.
        /*##-1- Configure the TIM peripheral #######################################*/
        uint32_t TIM_Prescaler = (uint32_t)((SystemCoreClock / TONE_TIM_COUNTER_CLOCK_FREQ) - 1);
        uint32_t TIM_CCR = (uint32_t)(TONE_TIM_COUNTER_CLOCK_FREQ / (2 * frequency));

        int32_t timer_channel_toggle_count = -1;
        // Calculate the toggle count
        if (duration > 0)
        {
            timer_channel_toggle_count = 2 * frequency * duration / 1000; // Change to seconds.
        }

        PIN_MAP[pin].timer_ccr = TIM_CCR;
        PIN_MAP[pin].user_property = timer_channel_toggle_count;

        TIM_OC_InitTypeDef sConfig;
        TimHandleTone.Instance = PIN_MAP[pin].timer_peripheral;
        if(PIN_MAP[pin].timer_peripheral == TIM2)
        {
            TimHandleTone.Init.Period        = 0xFFFFFFFF; // 32 bit timer 
        }
        else
        {
            TimHandleTone.Init.Period            = 0xFFFF; // 16 bit timer 65535
        }
        TimHandleTone.Init.Prescaler         = TIM_Prescaler;
        TimHandleTone.Init.ClockDivision     = 0;
        TimHandleTone.Init.CounterMode       = TIM_COUNTERMODE_UP;
        HAL_TIM_OC_Init(&TimHandleTone);

        /*##-2- Configure the Output Compare channels ##############################*/
        /* Common configuration for all channels */
        sConfig.OCMode       = TIM_OCMODE_TOGGLE;
        sConfig.OCPolarity   = TIM_OCPOLARITY_HIGH;
        /* Set the pulse value for channel 1 */
        sConfig.Pulse = TIM_CCR;


        if (HAL_TIM_OC_ConfigChannel(&TimHandleTone, &sConfig, PIN_MAP[pin].timer_ch) != HAL_OK)
        {
            /* Configuration Error */
            DEBUG("TIM OC CconfigChannel Error");
        }

        /* Start channel */
        if (HAL_TIM_OC_Start_IT(&TimHandleTone, PIN_MAP[pin].timer_ch) != HAL_OK)
        {
            //Error_Handler();
            DEBUG("TIM OC Start IT Error");
        }
    }
    else
    {
        // Error
        DEBUG("The Tone First Error");
    }
}

/**
 * @brief  Stop the Tone.
 * @param  pin: the selected pin.
 * @retval None
 */
void HAL_Tone_Stop(uint8_t pin)
{
    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    HAL_TIM_OC_Stop_IT(&TimHandleTone, PIN_MAP[pin].timer_ch);
    PIN_MAP[pin].timer_ccr = 0;
    PIN_MAP[pin].user_property = 0;
}

/**
 * @brief  Get the tone frequency.
 * @param  pin: the selected pin.
 * @retval The tone frequency.
 */
//HAL_Tone_Get_Frequency() should be called immediately after calling tone()
//"only" useful for unit-testing

uint32_t HAL_Tone_Get_Frequency(uint8_t pin)
{
    uint16_t TIM_CCR = 0;
    uint16_t Tone_Frequency = 0;

    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    if(PIN_MAP[pin].timer_ch == TIM_CHANNEL_1)
    {
        TIM_CCR = PIN_MAP[pin].timer_peripheral->CCR1;
    }
    else if(PIN_MAP[pin].timer_ch == TIM_CHANNEL_2)
    {
        TIM_CCR = PIN_MAP[pin].timer_peripheral->CCR2;
    }
    else if(PIN_MAP[pin].timer_ch == TIM_CHANNEL_3)
    {
        TIM_CCR = PIN_MAP[pin].timer_peripheral->CCR3;
    }
    else if(PIN_MAP[pin].timer_ch == TIM_CHANNEL_4)
    {
        TIM_CCR = PIN_MAP[pin].timer_peripheral->CCR4;
    }
    else
    {
        return Tone_Frequency;
    }

    Tone_Frequency = (uint16_t)(TONE_TIM_COUNTER_CLOCK_FREQ / TIM_CCR);

    return Tone_Frequency;
}

/**
 * @brief  Check the tone stopped or not.
 * @param  pin: the selected pin.
 * @retval true: stopped; false: running.
 */
bool HAL_Tone_Is_Stopped(uint8_t pin)
{
    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    if(PIN_MAP[pin].timer_ccr > 0)
    {
        return false;
    }

    return true;
}

/**
 * @brief  This function handles TIM2_INSTANCE Interrupt.
 * @param  None
 * @retval None
 */
void TIM2_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&TimHandleTone);
}
/**
 * @brief  This function handles TIM3_INSTANCE Interrupt.
 * @param  None
 * @retval None
 */
void TIM3_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&TimHandleTone);
}

/**
 * @brief  This function handles TIM4_INSTANCE Interrupt.
 * @param  None
 * @retval None
 */
void TIM9_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&TimHandleTone);
}

/**
 * @brief  This function handles TIM5_INSTANCE Interrupt.
 * @param  None
 * @retval None
 */
void TIM10_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&TimHandleTone);
}

void TIM11_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&TimHandleTone);
}
// CHANNEL_1: TIM11(D4), TIM9(A0), TIM3(A3)
// CHANNEL_2: TIM9(D3), TIM3(A2),
// CHANNEL_3: TIM2(D0)
// CHANNEL_4: TIM2(D1)

#define NONE 0xFF
#define TIM2_TONE_CHANNEL_1    NONE
#define TIM2_TONE_CHANNEL_2    NONE
#define TIM2_TONE_CHANNEL_3    D0
#define TIM2_TONE_CHANNEL_4    D1

#define TIM3_TONE_CHANNEL_1    A3
#define TIM3_TONE_CHANNEL_2    A2
#define TIM3_TONE_CHANNEL_3    NONE
#define TIM3_TONE_CHANNEL_4    NONE

#define TIM9_TONE_CHANNEL_1    A0
#define TIM9_TONE_CHANNEL_2    D3
#define TIM9_TONE_CHANNEL_3    NONE
#define TIM9_TONE_CHANNEL_4    NONE

#define TIM11_TONE_CHANNEL_1    D4
#define TIM11_TONE_CHANNEL_2    NONE
#define TIM11_TONE_CHANNEL_3    NONE
#define TIM11_TONE_CHANNEL_4    NONE

/**
 * @brief  Output Compare callback in non blocking mode
 * @param  htim : TIM OC handle
 * @retval None
 */
void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim)
{
    uint32_t uhCapture = 0;
    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();

    if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
    {
        uhCapture = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
        if(htim->Instance == TIM2)
        {
            if(PIN_MAP[TIM2_TONE_CHANNEL_1].user_property != -1)
            {
                /* Set the Capture Compare Register value */
                __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_1, (uhCapture + PIN_MAP[TIM2_TONE_CHANNEL_1].timer_ccr));
                if (PIN_MAP[TIM2_TONE_CHANNEL_1].user_property > 0)
                {
                    PIN_MAP[TIM2_TONE_CHANNEL_1].user_property -= 1;
                }
                else
                {
                    HAL_Tone_Stop(TIM2_TONE_CHANNEL_1);
                }
            }
        }
        else if(htim->Instance == TIM3)
        {

            if(PIN_MAP[TIM3_TONE_CHANNEL_1].user_property != -1)
            {
                /* Set the Capture Compare Register value */
                __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_1, (uhCapture + PIN_MAP[TIM3_TONE_CHANNEL_1].timer_ccr));
                if(PIN_MAP[TIM3_TONE_CHANNEL_1].user_property != -1)
                {
                    if (PIN_MAP[TIM3_TONE_CHANNEL_1].user_property > 0)
                    {
                        PIN_MAP[TIM3_TONE_CHANNEL_1].user_property -= 1;
                    }
                    else
                    {
                        HAL_Tone_Stop(TIM3_TONE_CHANNEL_1);
                    }
                }
            }
        }
        else if(htim->Instance == TIM9)
        {
            if(PIN_MAP[TIM9_TONE_CHANNEL_1].user_property != -1)
            {
                /* Set the Capture Compare Register value */
                __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_1, (uhCapture + PIN_MAP[TIM9_TONE_CHANNEL_1].timer_ccr));
                if (PIN_MAP[TIM9_TONE_CHANNEL_1].user_property > 0)
                {
                    PIN_MAP[TIM9_TONE_CHANNEL_1].user_property -= 1;
                }
                else
                {
                    HAL_Tone_Stop(TIM9_TONE_CHANNEL_1);
                }
            }
        }
        else if(htim->Instance == TIM11)
        {
            if(PIN_MAP[TIM11_TONE_CHANNEL_1].user_property != -1)
            {
                /* Set the Capture Compare Register value */
                __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_1, (uhCapture + PIN_MAP[TIM11_TONE_CHANNEL_1].timer_ccr));

                if (PIN_MAP[TIM11_TONE_CHANNEL_1].user_property > 0)
                {
                    PIN_MAP[TIM11_TONE_CHANNEL_1].user_property -= 1;
                }
                else
                {
                    HAL_Tone_Stop(TIM11_TONE_CHANNEL_1);
                }
            }
        }
    }
    else if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2)
    {
        uhCapture = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);
        if(htim->Instance == TIM2)
        {
            if(PIN_MAP[TIM2_TONE_CHANNEL_2].user_property != -1)
            {
                /* Set the Capture Compare Register value */
                __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_2, (uhCapture + PIN_MAP[TIM2_TONE_CHANNEL_2].timer_ccr));
                if (PIN_MAP[TIM2_TONE_CHANNEL_2].user_property > 0)
                {
                    PIN_MAP[TIM2_TONE_CHANNEL_2].user_property -= 1;
                }
                else
                {
                    HAL_Tone_Stop(TIM2_TONE_CHANNEL_2);
                }
            }
        }
        else if(htim->Instance == TIM3)
        {
            if(PIN_MAP[TIM3_TONE_CHANNEL_2].user_property != -1)
            {
                /* Set the Capture Compare Register value */
                __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_2, (uhCapture + PIN_MAP[TIM3_TONE_CHANNEL_2].timer_ccr));
                if (PIN_MAP[TIM3_TONE_CHANNEL_2].user_property > 0)
                {
                    PIN_MAP[TIM3_TONE_CHANNEL_2].user_property -= 1;
                }
                else
                {
                    HAL_Tone_Stop(TIM3_TONE_CHANNEL_2);
                }
            }
        }
        else if(htim->Instance == TIM9)
        {
            if(PIN_MAP[TIM9_TONE_CHANNEL_2].user_property != -1)
            {
                /* Set the Capture Compare Register value */
                __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_2, (uhCapture + PIN_MAP[TIM9_TONE_CHANNEL_2].timer_ccr));
                if (PIN_MAP[TIM9_TONE_CHANNEL_2].user_property > 0)
                {
                    PIN_MAP[TIM9_TONE_CHANNEL_2].user_property -= 1;
                }
                else
                {
                    HAL_Tone_Stop(TIM9_TONE_CHANNEL_2);
                }
            }
        }
        else if(htim->Instance == TIM11)
        {
            if(PIN_MAP[TIM11_TONE_CHANNEL_2].user_property != -1)
            {
                /* Set the Capture Compare Register value */
                __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_2, (uhCapture + PIN_MAP[TIM11_TONE_CHANNEL_2].timer_ccr));
                if (PIN_MAP[TIM11_TONE_CHANNEL_2].user_property > 0)
                {
                    PIN_MAP[TIM11_TONE_CHANNEL_2].user_property -= 1;
                }
                else
                {
                    HAL_Tone_Stop(TIM11_TONE_CHANNEL_2);
                }
            }
        }
    }
    else if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3)
    {
        uhCapture = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_3);
        if(htim->Instance == TIM2)
        {
            if(PIN_MAP[TIM2_TONE_CHANNEL_3].user_property != -1)
            {
                /* Set the Capture Compare Register value */
                __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_3, (uhCapture + PIN_MAP[TIM2_TONE_CHANNEL_3].timer_ccr));
                if (PIN_MAP[TIM2_TONE_CHANNEL_3].user_property > 0)
                {
                    PIN_MAP[TIM2_TONE_CHANNEL_3].user_property -= 1;
                }
                else
                {
                    HAL_Tone_Stop(TIM2_TONE_CHANNEL_3);
                }
            }
        }
        else if(htim->Instance == TIM3)
        {
            if(PIN_MAP[TIM3_TONE_CHANNEL_3].user_property != -1)
            {
                /* Set the Capture Compare Register value */
                __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_3, (uhCapture + PIN_MAP[TIM3_TONE_CHANNEL_3].timer_ccr));
                if (PIN_MAP[TIM3_TONE_CHANNEL_3].user_property > 0)
                {
                    PIN_MAP[TIM3_TONE_CHANNEL_3].user_property -= 1;
                }
                else
                {
                    HAL_Tone_Stop(TIM3_TONE_CHANNEL_3);
                }
            }
        }
        else if(htim->Instance == TIM9)
        {
            if(PIN_MAP[TIM9_TONE_CHANNEL_3].user_property != -1)
            {
                /* Set the Capture Compare Register value */
                __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_3, (uhCapture + PIN_MAP[TIM9_TONE_CHANNEL_3].timer_ccr));
                if (PIN_MAP[TIM9_TONE_CHANNEL_3].user_property > 0)
                {
                    PIN_MAP[TIM9_TONE_CHANNEL_3].user_property -= 1;
                }
                else
                {
                    HAL_Tone_Stop(TIM9_TONE_CHANNEL_3);
                }
            }
        }
        else if(htim->Instance == TIM11)
        {
            if(PIN_MAP[TIM11_TONE_CHANNEL_3].user_property != -1)
            {
                /* Set the Capture Compare Register value */
                __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_3, (uhCapture + PIN_MAP[TIM11_TONE_CHANNEL_3].timer_ccr));
                if (PIN_MAP[TIM11_TONE_CHANNEL_3].user_property > 0)
                {
                    PIN_MAP[TIM11_TONE_CHANNEL_3].user_property -= 1;
                }
                else
                {
                    HAL_Tone_Stop(TIM11_TONE_CHANNEL_3);
                }
            }
        }
    }
    else if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_4)
    {
        uhCapture = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_4);
        if(htim->Instance == TIM2)
        {
            if(PIN_MAP[TIM2_TONE_CHANNEL_4].user_property != -1)
            {
                /* Set the Capture Compare Register value */
                __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_4, (uhCapture + PIN_MAP[TIM2_TONE_CHANNEL_4].timer_ccr));
                if (PIN_MAP[TIM2_TONE_CHANNEL_4].user_property > 0)
                {
                    PIN_MAP[TIM2_TONE_CHANNEL_4].user_property -= 1;
                }
                else
                {
                    HAL_Tone_Stop(TIM2_TONE_CHANNEL_4);
                }
            }
        }
        else if(htim->Instance == TIM3)
        {
            if(PIN_MAP[TIM3_TONE_CHANNEL_4].user_property != -1)
            {
                /* Set the Capture Compare Register value */
                __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_4, (uhCapture + PIN_MAP[TIM3_TONE_CHANNEL_4].timer_ccr));
                if (PIN_MAP[TIM3_TONE_CHANNEL_4].user_property > 0)
                {
                    PIN_MAP[TIM3_TONE_CHANNEL_4].user_property -= 1;
                }
                else
                {
                    HAL_Tone_Stop(TIM3_TONE_CHANNEL_4);
                }
            }
        }
        else if(htim->Instance == TIM9)
        {
            if(PIN_MAP[TIM9_TONE_CHANNEL_4].user_property != -1)
            {
                /* Set the Capture Compare Register value */
                __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_4, (uhCapture + PIN_MAP[TIM9_TONE_CHANNEL_4].timer_ccr));
                if (PIN_MAP[TIM9_TONE_CHANNEL_4].user_property > 0)
                {
                    PIN_MAP[TIM9_TONE_CHANNEL_4].user_property -= 1;
                }
                else
                {
                    HAL_Tone_Stop(TIM9_TONE_CHANNEL_4);
                }
            }
        }
        else if(htim->Instance == TIM11)
        {
            if(PIN_MAP[TIM11_TONE_CHANNEL_4].user_property != -1)
            {
                /* Set the Capture Compare Register value */
                __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_4, (uhCapture + PIN_MAP[TIM11_TONE_CHANNEL_4].timer_ccr));
                if (PIN_MAP[TIM11_TONE_CHANNEL_4].user_property > 0)
                {
                    PIN_MAP[TIM11_TONE_CHANNEL_4].user_property -= 1;
                }
                else
                {
                    HAL_Tone_Stop(TIM11_TONE_CHANNEL_4);
                }
            }
        }
    }
}
