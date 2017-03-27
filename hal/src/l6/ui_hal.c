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
#include "hw_config.h"
#include "ui_hal.h"

void Set_RGB_Color(uint32_t color)
{
}

void RGB_Color_Toggle(void)
{
}

void HAL_UI_Initial(void)
{
}

uint8_t HAL_UI_Mode_BUTTON_GetState(Button_TypeDef Button)
{
    return 0;
}

uint32_t HAL_UI_Mode_Button_Pressed(void)
{
    return 0;
}

int HAL_UI_RGB_Get_Info(rgb_info_t *pinfo)
{
    return 0;
}

int HAL_UI_RGB_Set_Info(rgb_info_t info)
{
    return 0;
}

void HAL_UI_RGB_Color(uint32_t color)
{
}

void HAL_UI_RGB_Blink(uint32_t color, uint16_t period)
{
}

void HAL_UI_RGB_Breath(uint32_t color, uint16_t period)
{
}

void HAL_UI_UserLED_Control(uint8_t value)
{
}

void HAL_UI_SysTick_Handler(void)
{
}

