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
#include "hw_config.h"
#include "pinmap_impl.h"
#include "service_debug.h"

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

/*
 * @brief Attach the pin external interrupt with the defined handler and mode
 * @param pin: Pin to attach
 * @param handler: Call back function, when interrupt occurs, the handler will be call
 * @param data: The input data of handler
 * @param mode: The interrupt mode
 * This parameter can be one of following values:
 * @arg CHANGE
 * @arg RISING
 * @arg FALLING
 * @param config: not used
 * @retral None
 */
void HAL_Interrupts_Attach(uint16_t pin, HAL_InterruptHandler handler, void* data, InterruptMode mode,
        HAL_InterruptExtraConfiguration* config)
{
    uint8_t GPIO_PortSource = 0;    //variable to hold the port number

    //Map the pin to the appropriate port and pin on the STM32
    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    GPIO_TypeDef *gpio_port = PIN_MAP[pin].gpio_peripheral;
    uint16_t gpio_pin = PIN_MAP[pin].gpio_pin;
    uint8_t GPIO_PinSource = PIN_MAP[pin].gpio_pin_source;

    __HAL_GPIO_EXTI_CLEAR_FLAG(gpio_pin);
    //Select the port source
    if (gpio_port == GPIOA) {
        __HAL_RCC_GPIOA_CLK_ENABLE();
    } else if (gpio_port == GPIOB) {
        __HAL_RCC_GPIOB_CLK_ENABLE();
    } else if (gpio_port == GPIOC) {
        __HAL_RCC_GPIOC_CLK_ENABLE();
    } else if (gpio_port == GPIOD) {
        __HAL_RCC_GPIOD_CLK_ENABLE();
    }

    // Register the handler for the user function name
    exti_channels[GPIO_PinSource].fn = handler;
    exti_channels[GPIO_PinSource].data = data;

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    GPIO_InitStructure.Pin = gpio_pin;

    switch (mode) {
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

    if (gpio_port == GPIOA) {
        HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);
    } else if (gpio_port == GPIOB ) {
        HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);
    } else if (gpio_port == GPIOC ) {
        HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);
    } else if (gpio_port == GPIOD ) {
        HAL_GPIO_Init(GPIOD, &GPIO_InitStructure);
    }

    if(NULL == config) {
        /* Enable and set EXTI line Interrupt to the lowest priority */
        HAL_NVIC_SetPriority( GPIO_IRQn[GPIO_PinSource], 14, 0);
        HAL_NVIC_EnableIRQ( GPIO_IRQn[GPIO_PinSource] );
    } else {
        /* Enable and set EXTI line Interrupt to the lowest priority */
        HAL_NVIC_SetPriority( GPIO_IRQn[GPIO_PinSource], config->IRQChannelPreemptionPriority, config->IRQChannelSubPriority);
        HAL_NVIC_EnableIRQ( GPIO_IRQn[GPIO_PinSource] );
    }
}

/*
 * @brief Detch the interrupts pin.
 * @param pin: Pin to be detched.
 * @retral None
 */
void HAL_Interrupts_Detach(uint16_t pin)
{
    //Map the pin to the appropriate pin on the STM32
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

/*
 * @brief Enable all the interrupts.
 * @param None
 * @retral None
 */
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
    // TODO add osThreadResumeAll()
    //osThreadResumeAll();
}

/*
 * @brief Disable all the interrupts.
 * @param None
 * @retral None
 */
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
    // TODO add osThreadSuspendAll
    //osThreadSuspendAll();
}

/*
 * @brief Suspend the interrupts.
 * @param None
 * @retral None
 */
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

/*
 * @brief Restore the interrupts.
 * @param None
 * @retral None
 */
void HAL_Interrupts_Restore(void)
{
    EXTI->IMR = exti_saved_state.imr;
    EXTI->EMR = exti_saved_state.emr;
    EXTI->RTSR = exti_saved_state.rtsr;
    EXTI->FTSR = exti_saved_state.ftsr;
}

/*
 * @brief EXTI Handler
 *        This function is called by any of the interrupt handlers.
 *        It essentially fetches the user function pointer from the array.
 * @param EXTI_Line: EXTI line
 * @retral None
 */
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
    int is = __get_PRIMASK();
    __disable_irq();
    return is;
}

void HAL_enable_irq(int is)
{
    if ((is & 1) == 0) {
        __enable_irq();
    }
}

/**
  * @brief  This function handles External line 0 interrupt request.
  * @param  None
  * @retval None
  */
void EXTI0_IRQHandler(void)
{
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_0) != RESET)
    {
        /* Clear the EXTI line pending bit */
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_0);

        if(NULL != HAL_EXTI_Handler)
        {
            HAL_EXTI_Handler(0);
        }
    }
}

/**
  * @brief  This function handles External line 1 interrupt request.
  * @param  None
  * @retval None
  */
void EXTI1_IRQHandler(void)
{
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_1) != RESET)
    {
        /* Clear the EXTI line pending bit */
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_1);

        if(NULL != HAL_EXTI_Handler)
        {
            HAL_EXTI_Handler(1);
        }
    }
}

/**
  * @brief  This function handles External line 2 interrupt request.
  * @param  None
  * @retval None
  */
void EXTI2_IRQHandler(void)
{
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_2) != RESET)
    {
        /* Clear the EXTI line pending bit */
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_2);

        if(NULL != HAL_EXTI_Handler)
        {
            HAL_EXTI_Handler(2);
        }
    }
}

/**
  * @brief  This function handles External line 3 interrupt request.
  * @param  None
  * @retval None
  */
void EXTI3_IRQHandler(void)
{
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_3) != RESET)
    {
        /* Clear the EXTI line pending bit */
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_3);
        if(NULL != HAL_EXTI_Handler)
        {
            HAL_EXTI_Handler(3);
        }
    }
}

/**
  * @brief  This function handles External line 4 interrupt request.
  * @param  None
  * @retval None
  */
void EXTI4_IRQHandler(void)
{
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_4) != RESET)
    {
        /* Clear the EXTI line pending bit */
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_4);

        if(NULL != HAL_EXTI_Handler)
        {
            HAL_EXTI_Handler(4);
        }
    }
}

/**
  * @brief  This function handles External line 5-9 interrupt request.
  * @param  None
  * @retval None
  */
void EXTI9_5_IRQHandler(void)
{
    //GPIO_PIN_8 and GPIO_PIN_9 support is not required for CORE_V02

    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_5) != RESET)
    {
        /* Clear the EXTI line pending bit */
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_5);

        if(NULL != HAL_EXTI_Handler)
        {
            HAL_EXTI_Handler(5);
        }
    }

    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_6) != RESET)
    {
        /* Clear the EXTI line pending bit */
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_6);

        if(NULL != HAL_EXTI_Handler)
        {
            HAL_EXTI_Handler(6);
        }
    }

    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_7) != RESET)
    {
        /* Clear the EXTI line pending bit */
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_7);

        if(NULL != HAL_EXTI_Handler)
        {
            HAL_EXTI_Handler(7);
        }
    }

    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_8) != RESET)
    {
        /* Clear the EXTI line pending bit */
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_8);

        if(NULL != HAL_EXTI_Handler)
        {
            HAL_EXTI_Handler(8);
        }
    }

    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_9) != RESET)
    {
        /* Clear the EXTI line pending bit */
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_9);

        if(NULL != HAL_EXTI_Handler)
        {
            HAL_EXTI_Handler(9);
        }
    }
}

/**
  * @brief  This function handles External line 10-15 interrupt request.
  * @param  None
  * @retval None
  */
void EXTI15_10_IRQHandler(void)
{
    //GPIO_PIN_10 and GPIO_PIN_12 support is not required for CORE_V02
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_10) != RESET)
    {
        /* Clear the EXTI line pending bit */
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_10);

        if(NULL != HAL_EXTI_Handler)
        {
            HAL_EXTI_Handler(10);
        }
    }

    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_11) != RESET)
    {
        /* Clear the EXTI line pending bit */
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_11);

        if(NULL != HAL_EXTI_Handler)
        {
            HAL_EXTI_Handler(11);
        }
    }

    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_12) != RESET)
    {
        /* Clear the EXTI line pending bit */
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_12);

        if(NULL != HAL_EXTI_Handler)
        {
            HAL_EXTI_Handler(12);
        }
    }

    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_13) != RESET)
    {
        /* Clear the EXTI line pending bit */
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_13);

        if(NULL != HAL_EXTI_Handler)
        {
            HAL_EXTI_Handler(13);
        }
    }

    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_14) != RESET)
    {
        /* Clear the EXTI line pending bit */
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_14);

        if(NULL != HAL_EXTI_Handler)
        {
            HAL_EXTI_Handler(14);
        }
    }

    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_15) != RESET)
    {
        /* Clear the EXTI line pending bit */
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_15);

        if(NULL != HAL_EXTI_Handler)
        {
            HAL_EXTI_Handler(15);
        }
    }
}

inline bool isISR()
{
	return (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0;
}

uint8_t HAL_IsISR()
{
	return isISR();
}

/* For now, we remember only one handler, but in future this may be extended to a
 * dynamically linked list to allow for multiple handlers.
 */
static HAL_InterruptCallback SystemInterruptHandlers[__Last_irq];


inline bool is_valid_irq(hal_irq_t irq)
{
    return irq<__Last_irq;
}

uint8_t HAL_Set_System_Interrupt_Handler(hal_irq_t irq, const HAL_InterruptCallback* callback, HAL_InterruptCallback* previous, void* reserved)
{
    if (!is_valid_irq(irq))
        return false;
    HAL_InterruptCallback cb = SystemInterruptHandlers[irq];
    if (previous)
        *previous = cb;
    if (callback)
        cb = *callback;
    else
        cb.handler = 0;

    return true;
}

uint8_t HAL_Get_System_Interrupt_Handler(hal_irq_t irq, HAL_InterruptCallback* callback, void* reserved)
{
    if (!is_valid_irq(irq))
        return false;

    if (callback) {
        HAL_InterruptCallback cb = SystemInterruptHandlers[irq];
        *callback = cb;
    }

    return true;
}

void HAL_System_Interrupt_Trigger(hal_irq_t irq, void* reserved)
{
    if (is_valid_irq(irq))
    {
        HAL_InterruptCallback cb = SystemInterruptHandlers[irq];
        if (cb.handler)
            cb.handler(cb.data);
    }
}
