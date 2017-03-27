/**
 ******************************************************************************
 * @file    main.cpp

 * @version V1.0.0
 * @date    13-March-2013
 *
 * Updated: 14-Feb-2014 David Sidrane <david_s5@usa.net>
 *
 * @brief   Main program body.
 ******************************************************************************
 Copyright (c) 2013-2015 IntoRobot Industries, Inc.  All rights reserved.

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation, either
 version 3 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this program; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "system_event.h"
#include "system_mode.h"
#include "system_task.h"
#include "system_network.h"
#include "system_network_internal.h"
#include "system_sleep.h"
#include "system_threading.h"
#include "system_user.h"
#include "system_update.h"
#include "core_hal.h"
#include "delay_hal.h"
#include "syshealth_hal.h"
#include "watchdog_hal.h"
#include "usb_hal.h"
#include "system_mode.h"
#include "system_rgbled.h"
#include "params_hal.h"
#include "wiring_interrupts.h"
#include "wiring_cellular.h"
#include "wiring_cellular_printable.h"
#include "wiring_system.h"
#include "system_version.h"
#include "system_product.h"
#include "system_config.h"
#include "service_debug.h"
#include "platforms.h"

using namespace intorobot;

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
typedef enum
{
    BUTTON_MODE_NONE = 0,
    BUTTON_MODE_CONFIG_SERIAL,
    BUTTON_MODE_CONFIG_IMLINK_SERIAL,
    BUTTON_MODE_CONFIG_AP_SERIAL,
    BUTTON_MODE_DEFFW,
    BUTTON_MODE_COM,
    BUTTON_MODE_FAC,
    BUTTON_MODE_NC,
    BUTTON_MODE_RESET
} button_mode_t;

volatile button_mode_t BUTTON_Mode = BUTTON_MODE_NONE;
volatile uint32_t BUTTON_press_time;


extern "C" void HAL_SysTick_Handler(void)
{
#ifndef configNO_SETUPBUTTON_UI

#if PLATFORM_ID == PLATFORM_ATOM || PLATFORM_ID == PLATFORM_NEUTRON || PLATFORM_ID == PLATFORM_NUT
#define TIMING_MODE_CONFIG_IMLINK_SERIAL           3000   //进入配置模式判断时间
#define TIMING_MODE_DEFAULT_RESTORE                7000   //进入默认固件灯程序升级判断时间
#define TIMING_MODE_SERIAL_COM                     10000  //进入串口转接程序判断时间
#define TIMING_MODE_FACTORY_RESET                  13000  //恢复出厂程序判断时间 不清空密钥
#define TIMING_MODE_NC                             20000  //无操作判断时间
#define TIMING_MODE_ALL_RESET                      30000  //完全恢复出厂判断时间 清空密钥

    BUTTON_press_time = HAL_UI_Mode_Button_Pressed();
    if(BUTTON_press_time) {
        if( BUTTON_press_time > TIMING_MODE_ALL_RESET ) {
            if(BUTTON_Mode!=BUTTON_MODE_RESET) {
                BUTTON_Mode=BUTTON_MODE_RESET; //恢复出厂设置  清除密钥
                HAL_UI_RGB_Color(RGB_COLOR_YELLOW);//黄灯打开
            }
        }
        else if( BUTTON_press_time > TIMING_MODE_NC ) {
            if(BUTTON_Mode!=BUTTON_MODE_NC) {
                BUTTON_Mode=BUTTON_MODE_NC; //退出
                HAL_UI_RGB_Color(RGB_COLOR_BLACK);//关灯
            }
        }
        else if( BUTTON_press_time > TIMING_MODE_FACTORY_RESET ) {
            if(BUTTON_Mode!=BUTTON_MODE_FAC) {
                BUTTON_Mode=BUTTON_MODE_FAC;//恢复出厂设置  不清除密钥
                HAL_UI_RGB_Color(RGB_COLOR_CYAN);//浅蓝灯打开
            }
        }
        else if( BUTTON_press_time > TIMING_MODE_SERIAL_COM ) {
            if(BUTTON_Mode!=BUTTON_MODE_COM) {
                BUTTON_Mode=BUTTON_MODE_COM;//进入串口转发程序
                HAL_UI_RGB_Color(RGB_COLOR_BLUE);//蓝灯打开
            }
        }
        else if( BUTTON_press_time > TIMING_MODE_DEFAULT_RESTORE ) {
            if(BUTTON_Mode!=BUTTON_MODE_DEFFW) {
                BUTTON_Mode=BUTTON_MODE_DEFFW; //恢复默认出厂程序
                HAL_UI_RGB_Color(RGB_COLOR_GREEN);//绿灯打开
            }
        }
        else if( BUTTON_press_time > TIMING_MODE_CONFIG_IMLINK_SERIAL ) {
            if(BUTTON_Mode!=BUTTON_MODE_CONFIG_IMLINK_SERIAL) {
                BUTTON_Mode=BUTTON_MODE_CONFIG_IMLINK_SERIAL; //wifi imlink配置模式
                HAL_UI_RGB_Color(RGB_COLOR_RED);//红灯打开
            }
        }
    } else {
        switch(BUTTON_Mode) {
            case BUTTON_MODE_CONFIG_IMLINK_SERIAL:
                if(SYSTEM_CONFIG_TYPE_IMLINK_SERIAL != System.configStatus()) {
                    System.configBegin(SYSTEM_CONFIG_TYPE_IMLINK_SERIAL);
                } else {
                    System.configEnd();
                }
                break;

            case BUTTON_MODE_DEFFW:
                HAL_Core_Enter_Firmware_Recovery_Mode();
                break;

            case BUTTON_MODE_COM:
                HAL_Core_Enter_Com_Mode();
                break;

            case BUTTON_MODE_FAC:
                HAL_Core_Enter_Factory_Reset_Mode();
                break;

            case BUTTON_MODE_NC:
                HAL_Core_System_Reset();
                break;

            case BUTTON_MODE_RESET:
                HAL_Core_Enter_Factory_All_Reset_Mode();
                break;

            default:
                break;
        }
        if(BUTTON_MODE_NONE != BUTTON_Mode) {
            BUTTON_Mode = BUTTON_MODE_NONE;
        }
    }
#elif PLATFORM_ID == PLATFORM_FIG

#define TIMING_MODE_CONFIG_IMLINK_SERIAL                2000   //进入ImLink配置模式判断时间
#define TIMING_MODE_CONFIG_AP_SERIAL                    5000   //进入Ap配置模式判断时间
#define TIMING_MODE_DEFAULT_RESTORE                     7000   //进入默认固件灯程序升级判断时间
#define TIMING_MODE_FACTORY_RESET                       13000  //恢复出厂程序判断时间 不清除密钥
#define TIMING_MODE_NC                                  20000  //无操作判断时间
#define TIMING_MODE_ALL_RESET                           30000  //完全恢复出厂判断时间 清空密钥

    BUTTON_press_time = HAL_UI_Mode_Button_Pressed();
    if(BUTTON_press_time) {
        if( BUTTON_press_time > TIMING_MODE_ALL_RESET ) {
            if(BUTTON_Mode!=BUTTON_MODE_RESET) {
                BUTTON_Mode=BUTTON_MODE_RESET; //恢复出厂设置  清除密钥
                HAL_UI_RGB_Color(RGB_COLOR_YELLOW);//黄灯打开
            }
        } else if( BUTTON_press_time > TIMING_MODE_NC ) {
            if(BUTTON_Mode!=BUTTON_MODE_NC) {
                BUTTON_Mode=BUTTON_MODE_NC; //退出
                HAL_UI_RGB_Color(RGB_COLOR_BLACK);//关灯
            }
        } else if( BUTTON_press_time > TIMING_MODE_FACTORY_RESET ) {
            if(BUTTON_Mode!=BUTTON_MODE_FAC) {
                BUTTON_Mode=BUTTON_MODE_FAC;//恢复出厂设置  不清除密钥
                HAL_UI_RGB_Color(RGB_COLOR_CYAN);//浅蓝灯打开
            }
        } else if( BUTTON_press_time > TIMING_MODE_DEFAULT_RESTORE ) {
            if(BUTTON_Mode!=BUTTON_MODE_DEFFW) {
                BUTTON_Mode=BUTTON_MODE_DEFFW; //恢复默认出厂程序
                HAL_UI_RGB_Color(RGB_COLOR_GREEN);//绿灯打开
            }
        } else if( BUTTON_press_time > TIMING_MODE_CONFIG_AP_SERIAL ) {
            if(BUTTON_Mode!=BUTTON_MODE_CONFIG_AP_SERIAL) {
                BUTTON_Mode=BUTTON_MODE_CONFIG_AP_SERIAL; //wifi ap配置模式
                HAL_UI_RGB_Color(RGB_COLOR_MAGENTA);//粉红打开
            }
        } else if( BUTTON_press_time > TIMING_MODE_CONFIG_IMLINK_SERIAL ) {
            if(BUTTON_Mode!=BUTTON_MODE_CONFIG_IMLINK_SERIAL) {
                BUTTON_Mode=BUTTON_MODE_CONFIG_IMLINK_SERIAL; //wifi imlink配置模式
                HAL_UI_RGB_Color(RGB_COLOR_RED);//红灯打开
            }
        }
    } else {
        switch(BUTTON_Mode) {
            case BUTTON_MODE_CONFIG_IMLINK_SERIAL:
                if(SYSTEM_CONFIG_TYPE_IMLINK_SERIAL != System.configStatus()) {
                    System.configBegin(SYSTEM_CONFIG_TYPE_IMLINK_SERIAL);
                } else {
                    System.configEnd();
                }
                break;

            case BUTTON_MODE_CONFIG_AP_SERIAL:
                if(SYSTEM_CONFIG_TYPE_AP_SERIAL != System.configStatus()) {
                    System.configBegin(SYSTEM_CONFIG_TYPE_AP_SERIAL);
                } else {
                    System.configEnd();
                }
                break;

            case BUTTON_MODE_DEFFW:
                HAL_Core_Enter_Firmware_Recovery_Mode();
                break;

            case BUTTON_MODE_FAC:
                HAL_Core_Enter_Factory_Reset_Mode();
                break;

            case BUTTON_MODE_NC:
                HAL_Core_System_Reset();
                break;

            case BUTTON_MODE_RESET:
                HAL_Core_Enter_Factory_All_Reset_Mode();
                break;

            default:
                break;
        }
        if(BUTTON_MODE_NONE != BUTTON_Mode) {
            BUTTON_Mode = BUTTON_MODE_NONE;
        }
    }

#elif PLATFORM_ID == PLATFORM_FOX || PLATFORM_ID == PLATFORM_ANT

#define TIMING_MODE_CONFIG_SERIAL                       3000   //进入串口配置模式判断时间
#define TIMING_MODE_DEFAULT_RESTORE                     7000   //进入默认固件灯程序升级判断时间
#define TIMING_MODE_FACTORY_RESET                       13000  //恢复出厂程序判断时间 不清除密钥

    BUTTON_press_time = HAL_UI_Mode_Button_Pressed();
    if(BUTTON_press_time) {
        if( BUTTON_press_time > TIMING_MODE_FACTORY_RESET ) {
            if(BUTTON_Mode!=BUTTON_MODE_FAC) {
                BUTTON_Mode=BUTTON_MODE_FAC;//恢复出厂设置  不清除密钥
                HAL_UI_RGB_Color(RGB_COLOR_CYAN);//浅蓝灯打开
            }
        } else if( BUTTON_press_time > TIMING_MODE_DEFAULT_RESTORE ) {
            if(BUTTON_Mode!=BUTTON_MODE_DEFFW) {
                BUTTON_Mode=BUTTON_MODE_DEFFW; //恢复默认出厂程序
                HAL_UI_RGB_Color(RGB_COLOR_GREEN);//绿灯打开
            }
        } else if( BUTTON_press_time > TIMING_MODE_CONFIG_SERIAL ) {
            if(BUTTON_Mode!=BUTTON_MODE_CONFIG_SERIAL) {
                BUTTON_Mode=BUTTON_MODE_CONFIG_SERIAL; //wifi imlink配置模式
                HAL_UI_RGB_Color(RGB_COLOR_RED);//红灯打开
            }
        }
    } else {
        switch(BUTTON_Mode) {
            case BUTTON_MODE_CONFIG_SERIAL:
                if(SYSTEM_CONFIG_TYPE_SERIAL != System.configStatus()) {
                    System.configBegin(SYSTEM_CONFIG_TYPE_SERIAL);
                } else {
                    System.configEnd();
                }
                break;

            case BUTTON_MODE_DEFFW:
                HAL_Core_Enter_Firmware_Recovery_Mode();
                break;

            case BUTTON_MODE_FAC:
                HAL_Core_Enter_Factory_Reset_Mode();
                break;

            default:
                break;
        }
        if(BUTTON_MODE_NONE != BUTTON_Mode) {
            BUTTON_Mode = BUTTON_MODE_NONE;
        }
    }

#endif

#endif
}

void app_loop(bool threaded)
{
    DECLARE_SYS_HEALTH(ENTERED_WLAN_Loop);
    if (!threaded)
        intorobot_process();

    static uint8_t INTOROBOT_WIRING_APPLICATION = 0;
    if ((INTOROBOT_WIRING_APPLICATION != 1)) {
        //Execute user application setup only once
        DECLARE_SYS_HEALTH(ENTERED_Setup);
        if (system_mode()!=SAFE_MODE)
            setup();
        INTOROBOT_WIRING_APPLICATION = 1;
        _post_loop();
    }

#ifdef configSETUP_ENABLE
    manage_system_config();
#endif

    // Execute user application loop
    DECLARE_SYS_HEALTH(ENTERED_Loop);
    if (system_mode()!=SAFE_MODE) {
        loop();
        DECLARE_SYS_HEALTH(RAN_Loop);
        _post_loop();
    }
}

static void load_system_fwlib_version(void)
{
    //保存固件库版本号
    char fw_ver1[32] = {0}, fw_ver2[32] = {0};
    system_version(fw_ver1);
    HAL_PARAMS_Get_System_fwlib_ver(fw_ver2, sizeof(fw_ver2));
    if(strcmp(fw_ver1, fw_ver2))
    {
        HAL_PARAMS_Set_System_fwlib_ver(fw_ver1);
        HAL_PARAMS_Save_Params();
    }
}

#if PLATFORM_THREADING

// This is the application loop ActiveObject.

void app_thread_idle()
{
    app_loop(true);
}

// don't wait to get items from the queue, so the application loop is processed as often as possible
// timeout after attempting to put calls into the application queue, so the system thread does not deadlock  (since the application may also
// be trying to put events in the system queue.)
ActiveObjectCurrentThreadQueue ApplicationThread(ActiveObjectConfiguration(app_thread_idle,
            0, /* take time */
            5000, /* put time */
            20 /* queue size */));

#endif

/*******************************************************************************
 * Function Name  : main.
 * Description    : main routine.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void app_setup_and_loop_initial(void)
{
    HAL_Core_Init();
    // We have running firmware, otherwise we wouldn't have gotten here
    DECLARE_SYS_HEALTH(ENTERED_Main);
    // load params
    load_system_fwlib_version();
    set_system_mode(DEFAULT);

    DEBUG("---------------welcome from IntoRobot!-----------------");

#if defined (START_DFU_FLASHER_SERIAL_SPEED) || defined (START_YMODEM_FLASHER_SERIAL_SPEED)
#ifdef configHAL_USB_CDC_ENABLE
    USB_USART_LineCoding_BitRate_Handler(system_lineCodingBitRateHandler);
#endif
#endif

#if 1
    product_details_t product_details;
    system_product_instance().get_product_details(product_details);
    DEBUG("product_firmware_version=%d", product_details .product_firmware_version);
    DEBUG("product_mode=%d", product_details.product_mode);
    DEBUG("product_id=%s", product_details.product_id);
    DEBUG("product_secret=%s", product_details.product_secret);
#endif

#ifdef configSETUP_ENABLE
    system_config_setup();
    //nut 调用连接直接进入imlink模式，配置不了。如果进入配置模式，不初始化连接   chenkaiyao 2016-12-16
    if( SYSTEM_CONFIG_TYPE_NONE == get_system_config_type() )
    {
        NEWORK_FN(Network_Setup(), (void)0);
    }
#else
    NEWORK_FN(Network_Setup(), (void)0);
#endif

    bool threaded = system_thread_get_state(NULL) != intorobot::feature::DISABLED &&
      (system_mode()!=SAFE_MODE);

#if PLATFORM_THREADING
    if (threaded)
    {
        SystemThread.start();
        ApplicationThread.start();
    }
    else
    {
        SystemThread.setCurrentThread();
        ApplicationThread.setCurrentThread();
    }
    //create_system_task();
#else
    HAL_Core_Set_System_Loop_Handler(&system_process_loop);
#endif
}

/*******************************************************************************
 * Function Name  : main.
 * Description    : main routine.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void app_setup_and_loop(void)
{
    app_setup_and_loop_initial();
#if !PLATFORM_THREADING
    /* Main loop */
    while (1) {
        app_loop(false);
    }
#endif
}

#ifdef USE_FULL_ASSERT
/*******************************************************************************
 * Function Name  : assert_failed
 * Description    : Reports the name of the source file and the source line number
 *                  where the assert_param error has occurred.
 * Input          : - file: pointer to the source file name
 *                  - line: assert_param error line source number
 * Output         : None
 * Return         : None
 *******************************************************************************/
void assert_failed(uint8_t* file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1)
    {
    }
}
#endif
