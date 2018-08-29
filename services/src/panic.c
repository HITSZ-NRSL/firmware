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

#include "intorobot_config.h"
#include "intorobot_macros.h"
#include <stdint.h>
#include "panic.h"
#include "molmc_log.h"
#include "ui_hal.h"
#include "delay_hal.h"
#include "watchdog_hal.h"
#include "interrupts_hal.h"
#include "core_hal.h"


#define LOOPSPERMSEC 5483

typedef struct {
        uint32_t  led;
        uint16_t  count;
} flash_codes_t;


#define def_panic_codes(_class,led,code) {led, code},
static const flash_codes_t flash_codes[] = {
                 {0,0}, //NotUsedPanicCode
#include "panic_codes.h"
};
#undef def_panic_codes

/****************************************************************************
* Public Functions
****************************************************************************/

void panic_(ePanicCode code, void* extraInfo, void (*HAL_Delay_Microseconds)(uint32_t))
{
#if 0
        HAL_disable_irq();
        // Flush any serial message to help the poor bugger debug this;
        flash_codes_t pcd = flash_codes[code];
        LED_SetRGBColor(RGB_COLOR_RED);
        LED_SetBrightness(DEFAULT_LED_RGB_BRIGHTNESS);
        LED_Signaling_Stop();
        uint16_t c;
        int loops = 2;
        log_direct_("!");
        LED_Off(LED_RGB);
        while(loops) {
                // preamble
            for (c = 3; c; c--) {
                LED_SetRGBColor(pcd.led);
                LED_On(LED_RGB);
                HAL_Delay_Microseconds(MS2u(150));
                LED_Off(LED_RGB);
                HAL_Delay_Microseconds(MS2u(100));
            }

            HAL_Delay_Microseconds(MS2u(100));
            for (c = 3; c; c--) {
                LED_SetRGBColor(pcd.led);
                LED_On(LED_RGB);
                HAL_Delay_Microseconds(MS2u(300));
                LED_Off(LED_RGB);
                HAL_Delay_Microseconds(MS2u(100));
            }
            HAL_Delay_Microseconds(MS2u(100));

            for (c = 3; c; c--) {
                LED_SetRGBColor(pcd.led);
                LED_On(LED_RGB);
                HAL_Delay_Microseconds(MS2u(150));
                LED_Off(LED_RGB);
                HAL_Delay_Microseconds(MS2u(100));
            }

            // pause
            HAL_Delay_Microseconds(MS2u(900));
            // play code
            for (c = code; c; c--) {
                LED_SetRGBColor(pcd.led);
                LED_On(LED_RGB);
                HAL_Delay_Microseconds(MS2u(300));
                LED_Off(LED_RGB);
                HAL_Delay_Microseconds(MS2u(300));
            }
            // pause
            HAL_Delay_Microseconds(MS2u(800));
#if defined(RELEASE_BUILD) || defined(PANIC_BUT_KEEP_CALM)
            if (--loops == 0) HAL_Core_System_Reset();
#endif
        }
#endif
}
