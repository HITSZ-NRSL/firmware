/**
 ******************************************************************************
 * @file    USB_Device/CDC_Standalone/Src/stm32f4xx_it.c
 * @author  MCD Application Team
 * @version V1.3.2
 * @date    13-November-2015
 * @brief   Main Interrupt Service Routines.
 *          This file provides template for all exceptions handler and
 *          peripherals interrupt service routine.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2015 STMicroelectronics</center></h2>
 *
 * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *        http://www.st.com/software_license_agreement_liberty_v2
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include "stm32f4xx.h"
#include "stm32f4xx_it.h"
#include "service_debug.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/


//extern PCD_HandleTypeDef hpcd;

/* UART handler declared in "usbd_cdc_interface.c" file */
extern UART_HandleTypeDef UartHandle;

/* TIM handler declared in "usbd_cdc_interface.c" file */
extern TIM_HandleTypeDef TimHandle;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
void HAL_USART1_Handler(UART_HandleTypeDef *huart) __attribute__ ((weak));
void Wiring_USART2_Interrupt_Handler(void) __attribute__ ((weak));
void WifiDrv_USART1_Interrupt_Handler(void);

/******************************************************************************/
/*            Cortex-M4 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
 * @brief  This function handles NMI exception.
 * @param  None
 * @retval None
 */
void NMI_Handler(void)
{
}

void UsageFault_Handler(void);
void HardFault_Handler( void ) __attribute__( ( naked ) );

__attribute__((externally_visible)) void prvGetRegistersFromStack( uint32_t *pulFaultStackAddress )
{
    /* These are volatile to try and prevent the compiler/linker optimising them
       away as the variables never actually get used.  If the debugger won't show the
       values of the variables, make them global my moving their declaration outside
       of this function. */
    volatile uint32_t r0;
    volatile uint32_t r1;
    volatile uint32_t r2;
    volatile uint32_t r3;
    volatile uint32_t r12;
    volatile uint32_t lr; /* Link register. */
    volatile uint32_t pc; /* Program counter. */
    volatile uint32_t psr;/* Program status register. */

    r0 = pulFaultStackAddress[ 0 ];
    r1 = pulFaultStackAddress[ 1 ];
    r2 = pulFaultStackAddress[ 2 ];
    r3 = pulFaultStackAddress[ 3 ];

    r12 = pulFaultStackAddress[ 4 ];
    lr = pulFaultStackAddress[ 5 ];
    pc = pulFaultStackAddress[ 6 ];
    psr = pulFaultStackAddress[ 7 ];

    /* Silence "variable set but not used" error */
    if (false) {
        (void)r0; (void)r1; (void)r2; (void)r3; (void)r12; (void)lr; (void)pc; (void)psr;
    }

    if (SCB->CFSR & (1<<25) /* DIVBYZERO */) {
        // stay consistent with the core and cause 5 flashes
        UsageFault_Handler();
    }
    else {
        //PANIC(HardFault,"HardFault");
        /* Go to infinite loop when Hard Fault exception occurs */
        while (1)
        {
        }
    }
}

void HardFault_Handler(void)
{
    DEBUG("HardFault_Handler");
    __asm volatile
    (
        " tst lr, #4                                                \n"
        " ite eq                                                    \n"
        " mrseq r0, msp                                             \n"
        " mrsne r0, psp                                             \n"
        " ldr r1, [r0, #24]                                         \n"
        " ldr r2, handler2_address_const                            \n"
        " bx r2                                                     \n"
        " handler2_address_const: .word prvGetRegistersFromStack    \n"
    );
}

/**
 * @brief  This function handles Memory Manage exception.
 * @param  None
 * @retval None
 */
void MemManage_Handler(void)
{
    /* Go to infinite loop when Memory Manage exception occurs */
    while (1)
    {
    }
}

/**
 * @brief  This function handles Bus Fault exception.
 * @param  None
 * @retval None
 */
void BusFault_Handler(void)
{
    /* Go to infinite loop when Bus Fault exception occurs */
    while (1)
    {
    }
}

/**
 * @brief  This function handles Usage Fault exception.
 * @param  None
 * @retval None
 */
void UsageFault_Handler(void)
{
    /* Go to infinite loop when Usage Fault exception occurs */
    while (1)
    {
    }
}

/**
 * @brief  This function handles SVCall exception.
 * @param  None
 * @retval None
 */
/*void SVC_Handler(void)
{
}
*/

/**
 * @brief  This function handles Debug Monitor exception.
 * @param  None
 * @retval None
 */
void DebugMon_Handler(void)
{
}

/**
 * @brief  This function handles PendSVC exception.
 * @param  None
 * @retval None
 */
/*
void PendSV_Handler(void)
{
}
*/

#if 0
/**
 * @brief  This function handles SysTick Handler.
 * @param  None
 * @retval None
 */
void SysTick_Handler(void)
{
    System1MsTick();
    /* Handle short and generic tasks for the device HAL on 1ms ticks */
    HAL_1Ms_Tick();
    HAL_SysTick_Handler();
    //HAL_System_Interrupt_Trigger(SysInterrupt_SysTick, NULL);
}
#endif
/******************************************************************************/
/*                 STM32F4xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f4xx.s).                                               */
/******************************************************************************/
#if 0
/**
 * @brief  This function handles USB-On-The-Go FS global interrupt request.
 * @param  None
 * @retval None
 */
void OTG_FS_IRQHandler(void)
{
    HAL_PCD_IRQHandler(&hpcd);
}

/**
 * @brief  This function handles USARTx interrupt request.
 * @param  None
 * @retval None
 */
void USART1_IRQHandler(void)
{
    //HAL_USART1_Handler();
}

/**
 * @brief  This function handles USARTx interrupt request.
 * @param  None
 * @retval None
 */
void USART2_IRQHandler(void)
{
    Wiring_USART2_Interrupt_Handler();
    //HAL_UART_IRQHandler(&UartHandle);
}

#endif


extern TIM_HandleTypeDef Timer2Handle;
extern TIM_HandleTypeDef Timer3Handle;
extern TIM_HandleTypeDef Timer4Handle;
extern TIM_HandleTypeDef Timer5Handle;
extern TIM_HandleTypeDef Timer9Handle;
extern TIM_HandleTypeDef Timer10Handle;
extern TIM_HandleTypeDef Timer11Handle;

void TIM1_BRK_TIM9_IRQHandler(void)
{
//    HAL_TIM_IRQHandler(&Timer9Handle);
}

void TIM1_UP_TIM10_IRQHandler(void)
{
//    HAL_TIM_IRQHandler(&Timer10Handle);
}

void TIM1_TRG_COM_TIM11_IRQHandler(void)
{
 //   HAL_TIM_IRQHandler(&Timer11Handle);
}


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
