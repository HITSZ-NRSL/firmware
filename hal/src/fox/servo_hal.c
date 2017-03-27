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
#include "servo_hal.h"
#include "gpio_hal.h"
#include "pinmap_impl.h"
/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

// XXX: SytemCoreClock = TIM_CLK here, Other board need to be changed.
// or set the CLK in the code below
#define SERVO_TIM_PRESCALER (uint16_t)(SystemCoreClock / 1000000) - 1       // To get TIM counter clock = 1MHz, TIM2-5 CLK = SystemCoreClock
#define SERVO_TIM_ARR       (uint16_t)(1000000 / configSERVO_TIM_PWM_FREQ) - 1    // To get PWM period = 20ms

void HAL_Servo_Attach(uint16_t pin)
{
    //Map the pin to the appropriate port and pin on the STM32
    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();

    if(PIN_MAP[pin].timer_peripheral != NULL || PIN_MAP[pin].timer_peripheral != TIM1)
    {
        /* Common configruation for all channels */
        GPIO_InitTypeDef GPIO_InitStruct;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLDOWN;
        GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;

        HAL_Pin_Mode(pin, AF_OUTPUT_PUSHPULL);

        if(PIN_MAP[pin].timer_peripheral == TIM2)
        {
            __HAL_RCC_TIM2_CLK_ENABLE();
            GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
            GPIO_InitStruct.Pin       = PIN_MAP[pin].gpio_pin;
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
        }
        else if(PIN_MAP[pin].timer_peripheral == TIM3)
        {
            __HAL_RCC_TIM3_CLK_ENABLE();
            GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
            GPIO_InitStruct.Pin       = PIN_MAP[pin].gpio_pin;
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
        }
        else if(PIN_MAP[pin].timer_peripheral == TIM4)
        {
            __HAL_RCC_TIM4_CLK_ENABLE();
            GPIO_InitStruct.Alternate = GPIO_AF2_TIM4;
            GPIO_InitStruct.Pin       = PIN_MAP[pin].gpio_pin;
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
        }
        else if(PIN_MAP[pin].timer_peripheral == TIM5)
        {
            __HAL_RCC_TIM5_CLK_ENABLE();
            GPIO_InitStruct.Alternate = GPIO_AF2_TIM5;
            GPIO_InitStruct.Pin       = PIN_MAP[pin].gpio_pin;
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
        }

        TIM_HandleTypeDef TimHandle;
        TIM_OC_InitTypeDef sConfig;

        TimHandle.Instance = PIN_MAP[pin].timer_peripheral;

        TimHandle.Init.Prescaler         = SERVO_TIM_PRESCALER;
        TimHandle.Init.Period            = SERVO_TIM_ARR;
        TimHandle.Init.ClockDivision     = 0;
        TimHandle.Init.CounterMode       = TIM_COUNTERMODE_UP;
        TimHandle.Init.RepetitionCounter = 0;
        HAL_TIM_PWM_Init(&TimHandle);

        sConfig.OCMode       = TIM_OCMODE_PWM1;
        sConfig.OCPolarity   = TIM_OCPOLARITY_HIGH;
        sConfig.OCFastMode   = TIM_OCFAST_DISABLE;
        sConfig.OCNPolarity  = TIM_OCNPOLARITY_HIGH;

        /* Set the pulse value for channel 1 */
        sConfig.Pulse = 0x00; // no pwm at start point
        HAL_TIM_PWM_ConfigChannel(&TimHandle, &sConfig, PIN_MAP[pin].timer_ch);

        /* Start channel */
        HAL_TIM_PWM_Start(&TimHandle, PIN_MAP[pin].timer_ch);
    }
}

void HAL_Servo_Detach(uint16_t pin)
{
    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    // TIM disable counter
    TIM_HandleTypeDef TimHandle;
    TimHandle.Instance = PIN_MAP[pin].timer_peripheral;

    HAL_TIM_PWM_Stop(&TimHandle,TIM_CHANNEL_ALL);
}

void HAL_Servo_Write_Pulse_Width(uint16_t pin, uint16_t pulseWidth)
{
    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();

    //SERVO_TIM_CCR = pulseWidth * (SERVO_TIM_ARR + 1) * configSERVO_TIM_PWM_FREQ / 1000000;
    uint16_t SERVO_TIM_CCR = pulseWidth;

    TIM_OC_InitTypeDef sConfig;

    sConfig.OCMode       = TIM_OCMODE_PWM1;
    sConfig.OCPolarity   = TIM_OCPOLARITY_HIGH;
    sConfig.OCFastMode   = TIM_OCFAST_DISABLE;
    sConfig.OCNPolarity  = TIM_OCNPOLARITY_HIGH;

    /* Set the pulse value for channel 1 */
    sConfig.Pulse = SERVO_TIM_CCR;

    TIM_HandleTypeDef TimHandle;
    TimHandle.Instance = PIN_MAP[pin].timer_peripheral;

    HAL_TIM_PWM_ConfigChannel(&TimHandle, &sConfig, PIN_MAP[pin].timer_ch);
    /* Start channel */
    HAL_TIM_PWM_Start(&TimHandle, PIN_MAP[pin].timer_ch);
}

uint16_t HAL_Servo_Read_Pulse_Width(uint16_t pin)
{
    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    uint16_t SERVO_TIM_CCR = 0x0000;

    TIM_HandleTypeDef htim;
    htim.Instance = PIN_MAP[pin].timer_peripheral;

    SERVO_TIM_CCR = HAL_TIM_ReadCapturedValue(&htim, PIN_MAP[pin].timer_ch);

    //pulseWidth = (SERVO_TIM_CCR * 1000000) / ((SERVO_TIM_ARR + 1) * configSERVO_TIM_PWM_FREQ);
    return SERVO_TIM_CCR;
}

uint16_t HAL_Servo_Read_Frequency(uint16_t pin)
{
    return 0;
}
