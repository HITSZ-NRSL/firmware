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
#include <string.h>
#include "hw_config.h"
#include "core_hal.h"
#include "core_hal_esp8266.h"
#include "rng_hal.h"
#include "ui_hal.h"
#include "updater_hal.h"
#include "gpio_hal.h"
#include "interrupts_hal.h"
#include "intorobot_macros.h"
#include "rtc_hal.h"
#include "delay_hal.h"
#include "params_hal.h"
#include "bkpreg_hal.h"
#include "Schedule.h"
#include "esp8266-hal-wifi.h"
#include "flash_map.h"
#include "macaddr_hal.h"

const static char *TAG = "hal-core";

/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/
void HAL_Core_Setup(void);

/* Private macro ------------------------------------------------------------*/
#define LOOP_TASK_PRIORITY 1
#define LOOP_QUEUE_SIZE    1
#define OPTIMISTIC_YIELD_TIME_US 16000

/* Private variables --------------------------------------------------------*/
struct rst_info resetInfo;
static os_timer_t systick_timer;

/* Extern variables ----------------------------------------------------------*/
int atexit(void (*func)()) __attribute__((weak));
int atexit(void (*func)()) {
    return 0;
}

extern "C" void ets_update_cpu_frequency(int freqmhz);
void preloop_update_frequency() __attribute__((weak));
void preloop_update_frequency() {
#if defined(F_CPU) && (F_CPU == 160000000L)
    REG_SET_BIT(0x3ff00014, BIT(0));
    ets_update_cpu_frequency(160);
#endif
}

extern void (*__init_array_start)(void);
extern void (*__init_array_end)(void);

extern "C" void __gdb_init() {}
extern "C" void gdb_init(void) __attribute__ ((weak, alias("__gdb_init")));

extern "C" void __gdb_do_break(){}
extern "C" void gdb_do_break(void) __attribute__ ((weak, alias("__gdb_do_break")));

cont_t g_cont __attribute__ ((aligned (16)));
static os_event_t g_loop_queue[LOOP_QUEUE_SIZE];

static uint32_t g_micros_at_task_start;

extern "C" void esp_yield()
{
    if (cont_can_yield(&g_cont)) {
        cont_yield(&g_cont);
    }
}

extern "C" void esp_schedule()
{
    ets_post(LOOP_TASK_PRIORITY, 0, 0);
}

extern "C" void __yield()
{
    if (cont_can_yield(&g_cont)) {
        esp_schedule();
        esp_yield();
    } else {
        //panic();
    }
}

extern "C" void yield(void) __attribute__ ((weak, alias("__yield")));

extern "C" void optimistic_yield(uint32_t interval_us)
{
    if (cont_can_yield(&g_cont) &&
        (system_get_time() - g_micros_at_task_start) > interval_us) {
        g_micros_at_task_start = system_get_time();
        yield();
    }
}

static uint8_t intorobot_app_initial_flag = 0;
static void loop_wrapper()
{
    bool threaded;
    preloop_update_frequency();
    if(!intorobot_app_initial_flag) {
        app_setup_and_loop_initial(&threaded);
        intorobot_app_initial_flag = 1;
    }
    app_loop(false);
    run_scheduled_functions();
    esp_schedule();
}

static void loop_task(os_event_t *events)
{
    g_micros_at_task_start = system_get_time();
    cont_run(&g_cont, &loop_wrapper);
    if (cont_check(&g_cont) != 0) {
       // panic();
    }
}

static void do_global_ctors(void)
{
    void (**p)(void) = &__init_array_end;
    while (p != &__init_array_start)
        (*--p)();
}

void init_done()
{
    gdb_init();
    do_global_ctors();
    HAL_Core_Config();
    HAL_Core_Setup();
    wlan_set_macaddr_when_init();
    esp_schedule();
}

extern "C" void user_init(void)
{
    struct rst_info *rtc_info_ptr = system_get_rst_info();
    memcpy((void *) &resetInfo, (void *) rtc_info_ptr, sizeof(resetInfo));
    uart_div_modify(0, UART_CLK_FREQ / (115200));

    //Disable UART interrupts
    system_set_os_print(0);
    U0IE = 0;
    U1IE = 0;
    timer1_isr_init();
    HAL_Interrupts_Initial();

    SysTick_Enable();
    cont_init(&g_cont);
    ets_task(loop_task, LOOP_TASK_PRIORITY, g_loop_queue, LOOP_QUEUE_SIZE);
    system_init_done_cb(&init_done);
}

void SysTick_Handler(void* arg)
{
    HAL_SysTick_Handler();
    HAL_UI_SysTick_Handler();
}

void HAL_Core_Init(void)
{
}

void HAL_Core_Config(void)
{
    //滴答定时器  //处理三色灯和模式处理
    os_timer_setfn(&systick_timer, (os_timer_func_t*)&SysTick_Handler, 0);
    os_timer_arm(&systick_timer, 1, 1);

    //设置管脚的默认值
    for (pin_t pin = FIRST_DIGITAL_PIN; pin <= FIRST_DIGITAL_PIN + TOTAL_DIGITAL_PINS; pin++) {
        //HAL_Pin_Mode(pin, INPUT);  //暂时不初始化 初始化会重设串口，导致无法打印调试信息  robin 2017-09-08
    }

    HAL_RTC_Initial();
    HAL_RNG_Initial();

    HAL_IWDG_Initial();
    HAL_UI_Initial();
}

void HAL_Core_Load_Params(void)
{
    //加载bootloader和系统参数
    HAL_PARAMS_Load_System_Params();
    HAL_PARAMS_Load_Boot_Params();

    if(INITPARAM_FLAG_FACTORY_RESET == HAL_PARAMS_Get_Boot_initparam_flag()) {
        //初始化参数 保留密钥
        MOLMC_LOGD(TAG, "init params fac");
        HAL_PARAMS_Init_Fac_System_Params();
    } else if(INITPARAM_FLAG_ALL_RESET == HAL_PARAMS_Get_Boot_initparam_flag()) {
        //初始化所有参数
        MOLMC_LOGD(TAG, "init params all");
        HAL_PARAMS_Init_All_System_Params();
    }

    if(INITPARAM_FLAG_NORMAL != HAL_PARAMS_Get_Boot_initparam_flag()) {
        HAL_PARAMS_Set_Boot_initparam_flag(INITPARAM_FLAG_NORMAL);
        HAL_PARAMS_Save_Params();
    }
}

void HAL_Core_Setup(void)
{
    HAL_IWDG_Config(DISABLE);
    HAL_Core_Load_Params();
}

extern "C" void __real_system_restart_local();
void HAL_Core_System_Reset(void)
{
    HAL_Core_Write_Backup_Register(BKP_DR_03, 0x7DEA);
    __real_system_restart_local();
}

void HAL_Core_Enter_DFU_Mode(bool persist)
{
}

void HAL_Core_Enter_Com_Mode(void)
{
    HAL_PARAMS_Set_Boot_boot_flag(BOOT_FLAG_SERIAL_COM);
    HAL_PARAMS_Save_Params();
    HAL_Core_System_Reset();
}

//恢复出厂设置 不清除密钥
void HAL_Core_Enter_Factory_Reset_Mode(void)
{
    HAL_PARAMS_Set_Boot_boot_flag(BOOT_FLAG_FACTORY_RESET);
    HAL_PARAMS_Save_Params();
    HAL_Core_System_Reset();
}

void HAL_Core_Enter_Ota_Update_Mode(void)
{
    HAL_PARAMS_Set_Boot_boot_flag(BOOT_FLAG_OTA_UPDATE);
    HAL_PARAMS_Save_Params();
    HAL_Core_System_Reset();
}

void HAL_Core_Enter_Bootloader(bool persist)
{
}

void HAL_Core_Enter_Stop_Mode(uint16_t wakeUpPin, uint16_t edgeTriggerMode, long seconds)
{
}

void HAL_Core_Execute_Stop_Mode(void)
{
}

void HAL_Core_Enter_Standby_Mode(void)
{
}

void HAL_Core_Execute_Standby_Mode(void)
{
}

void HAL_Core_System_Yield(void)
{
    optimistic_yield(100);
}

uint32_t HAL_Core_Runtime_Info(runtime_info_t* info, void* reserved)
{
    info->freeheap = system_get_free_heap_size();
    return 0;
}

void HAL_Core_Enter_Config(void)
{
}

void HAL_Core_Exit_Config(void)
{
}
