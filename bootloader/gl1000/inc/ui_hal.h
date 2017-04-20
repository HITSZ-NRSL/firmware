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

#ifndef RGBLED_HAL_H
#define RGBLED_HAL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    BUTTON1 = 0
} Button_TypeDef;

typedef enum
{
    BUTTON_MODE_GPIO = 0, BUTTON_MODE_EXTI = 1
} ButtonMode_TypeDef;

typedef enum
{
    RGB_MODE_COLOR = 0,
    RGB_MODE_BLINK = 1,
    RGB_MODE_BREATH = 2
}rgb_mode_t;

typedef struct {
    rgb_mode_t  rgb_mode;
    uint8_t     rgb_fade_step;
    uint8_t     rgb_fade_direction;
    uint16_t    rgb_period;
    uint32_t    rgb_color;
    uint32_t    rgb_last_color;
}rgb_info_t;


//RGB Basic Colors
#define RGB_COLOR_BLACK     0x000000
#define RGB_COLOR_RED       0xFF0000
#define RGB_COLOR_GREEN     0x00FF00
#define RGB_COLOR_BLUE      0x0000FF
#define RGB_COLOR_YELLOW    0xFFFF00
#define RGB_COLOR_CYAN      0x00FFFF  //浅蓝色
#define RGB_COLOR_MAGENTA   0xFF00FF
#define RGB_COLOR_WHITE     0xFFFFFF
#define RGB_COLOR_ORANGE    0xFF6000
#define RGB_COLOR_GREY      0x1F1F1F


void HAL_UI_Initial(void);
uint8_t HAL_UI_Mode_BUTTON_GetState(Button_TypeDef Button);
uint32_t HAL_UI_Mode_Button_Pressed(void);
int HAL_UI_RGB_Get_Info(rgb_info_t *pinfo);
int HAL_UI_RGB_Set_Info(rgb_info_t info);
void HAL_UI_RGB_Color(uint32_t color);
void HAL_UI_RGB_Blink(uint32_t color, uint16_t period);
void HAL_UI_RGB_Breath(uint32_t color, uint16_t period);
void HAL_UI_UserLED_Control(uint8_t value);
void HAL_UI_SysTick_Handler(void);


#ifdef __cplusplus
}
#endif

#endif/* RGBLED_HAL_H */

