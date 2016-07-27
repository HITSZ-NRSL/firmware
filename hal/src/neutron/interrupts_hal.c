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
#include "interrupts_hal.h"
#include "stm32f4xx.h"
#include "pinmap_impl.h"
//Interrupts
static const uint8_t GPIO_IRQn[] = {
    EXTI0_IRQn,     //0
    EXTI1_IRQn,     //1
    EXTI2_IRQn,     //2
    EXTI3_IRQn,     //3
    EXTI4_IRQn,     //4
    EXTI9_5_IRQn,   //5
    EXTI9_5_IRQn,   //6
    EXTI9_5_IRQn,   //7
    EXTI9_5_IRQn,   //8
    EXTI9_5_IRQn,   //9
    EXTI15_10_IRQn, //10
    EXTI15_10_IRQn, //11
    EXTI15_10_IRQn, //12
    EXTI15_10_IRQn, //13
    EXTI15_10_IRQn, //14
    EXTI15_10_IRQn  //15
};

// Create a structure for user ISR function pointers
typedef struct exti_channel {
    HAL_InterruptHandler fn;
    void* data;
} exti_channel;

//Array to hold user ISR function pointers
static exti_channel exti_channels[16];

typedef struct exti_state {
    uint32_t imr;
    uint32_t emr;
    uint32_t rtsr;
    uint32_t ftsr;
} exti_state;

static exti_state exti_saved_state;

void HAL_Interrupts_Attach(uint16_t pin, HAL_InterruptHandler handler, void* data, InterruptMode mode,
        HAL_InterruptExtraConfiguration* config)
{
    uint8_t GPIO_PortSource = 0;    //variable to hold the port number

    //Map the Spark pin to the appropriate port and pin on the STM32
    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    GPIO_TypeDef *gpio_port = PIN_MAP[pin].gpio_peripheral;
    uint16_t gpio_pin = PIN_MAP[pin].gpio_pin;
    uint8_t GPIO_PinSource = PIN_MAP[pin].gpio_pin_source;

    __HAL_GPIO_EXTI_CLEAR_FLAG(gpio_pin);
    //Select the port source
    if (gpio_port == GPIOA)
    {
        __HAL_RCC_GPIOA_CLK_ENABLE();
    }
    else if (gpio_port == GPIOB)
    {
        __HAL_RCC_GPIOB_CLK_ENABLE();
    }

    // Register the handler for the user function name
    exti_channels[GPIO_PinSource].fn = handler;
    exti_channels[GPIO_PinSource].data = data;

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    GPIO_InitStructure.Pin = gpio_pin;

    switch (mode)
    {
        case CHANGE:
            //generate interrupt on rising or falling edge
            GPIO_InitStructure.Mode = GPIO_MODE_IT_RISING_FALLING;
            break;
        case RISING:
            //generate interrupt on rising edge
            GPIO_InitStructure.Mode = GPIO_MODE_IT_RISING;
            break;
        case FALLING:
            //generate interrupt on falling edge
            GPIO_InitStructure.Mode = GPIO_MODE_IT_FALLING;
            break;
    }

    if (gpio_port == GPIOA )
    {
        HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);
    }
    else if (gpio_port == GPIOB )
    {
        HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);
    }

    /* Enable and set EXTI line Interrupt to the lowest priority */
    HAL_NVIC_SetPriority( GPIO_IRQn[GPIO_PinSource],13, 0); // 14 or 13, which one
    HAL_NVIC_EnableIRQ( GPIO_IRQn[GPIO_PinSource] );
}

void HAL_Interrupts_Detach(uint16_t pin)
{
    //Map the Spark Core pin to the appropriate pin on the STM32
    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    uint16_t gpio_pin = PIN_MAP[pin].gpio_pin;
    uint8_t GPIO_PinSource = PIN_MAP[pin].gpio_pin_source;

    //Clear the pending interrupt flag for that interrupt pin
    __HAL_GPIO_EXTI_CLEAR_IT(gpio_pin);

    HAL_GPIO_DeInit(PIN_MAP[pin].gpio_peripheral, gpio_pin);
    HAL_NVIC_DisableIRQ(GPIO_IRQn[GPIO_PinSource]);

    //unregister the user's handler
    exti_channels[GPIO_PinSource].fn = NULL;
    exti_channels[GPIO_PinSource].data = NULL;
}

void HAL_Interrupts_Enable_All(void)
{
    //Only enable the interrupts that are exposed to the user
    NVIC_EnableIRQ(EXTI0_IRQn);
    NVIC_EnableIRQ(EXTI1_IRQn);
    NVIC_EnableIRQ(EXTI2_IRQn);
    NVIC_EnableIRQ(EXTI3_IRQn);
    NVIC_EnableIRQ(EXTI4_IRQn);
    NVIC_EnableIRQ(EXTI9_5_IRQn);
    NVIC_EnableIRQ(EXTI15_10_IRQn);
    osThreadResumeAll();
}

void HAL_Interrupts_Disable_All(void)
{
    //Only disable the interrupts that are exposed to the user
    NVIC_DisableIRQ(EXTI0_IRQn);
    NVIC_DisableIRQ(EXTI1_IRQn);
    NVIC_DisableIRQ(EXTI2_IRQn);
    NVIC_DisableIRQ(EXTI3_IRQn);
    NVIC_DisableIRQ(EXTI4_IRQn);
    NVIC_DisableIRQ(EXTI9_5_IRQn);
    NVIC_DisableIRQ(EXTI15_10_IRQn);
    osThreadSuspendAll();
}

void HAL_Interrupts_Suspend(void)
{
    exti_saved_state.imr = EXTI->IMR;
    exti_saved_state.emr = EXTI->EMR;
    exti_saved_state.rtsr = EXTI->RTSR;
    exti_saved_state.ftsr = EXTI->FTSR;

    EXTI->IMR = 0x00000000;
    EXTI->EMR = 0x00000000;
    EXTI->RTSR = 0x00000000;
    EXTI->FTSR = 0x00000000;
    EXTI->PR = 0x007FFFFF;
}

void HAL_Interrupts_Restore(void)
{
    EXTI->IMR = exti_saved_state.imr;
    EXTI->EMR = exti_saved_state.emr;
    EXTI->RTSR = exti_saved_state.rtsr;
    EXTI->FTSR = exti_saved_state.ftsr;
}

/*******************************************************************************
 * Function Name  : HAL_EXTI_Handler (Declared as weak in stm32_it.cpp)
 * Description    : This function is called by any of the interrupt handlers. It
                                                 essentially fetches the user function pointer from the array
                                                 and calls it.
 * Input          : EXTI_Line (Supported: 0, 1, 3, 4, 5, 6, 7, 13, 14 and 15)
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void HAL_EXTI_Handler(uint8_t EXTI_Line)
{
    //fetch the user function pointer from the array
    void* data = exti_channels[EXTI_Line].data;
    HAL_InterruptHandler userISR_Handle = exti_channels[EXTI_Line].fn;

    if (userISR_Handle)
    {
        userISR_Handle(data);
    }
}


int HAL_disable_irq()
{
    return 0;
}

void HAL_enable_irq(int is)
{
}

