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

#include "wiring_rgb.h"

RGBClass::RGBClass() {
    _control=false;
}

RGBClass::~RGBClass() {
}

bool RGBClass::controlled(void) {
    return _control;
}

void RGBClass::control(bool flag) {
    _control=flag;
}

bool RGBClass::off(void) {
    if(!_control) {
        return false;
    }

    HAL_UI_RGB_Color(RGB_COLOR_BLACK);
    return true;
}

bool RGBClass::color(uint32_t rgb) {
    if(!_control) {
        return false;
    }

    HAL_UI_RGB_Color(rgb);
    return true;
}

bool RGBClass::color(uint8_t red, uint8_t green, uint8_t blue) {
    return color((red<<16)|(green<<8)|blue);
}

bool RGBClass::blink(uint32_t rgb, uint16_t period) {
    if(!_control) {
        return false;
    }

    HAL_UI_RGB_Blink(rgb, period);
    return true;
}

bool RGBClass::blink(uint8_t red, uint8_t green, uint8_t blue, uint16_t period) {
    return blink((red<<16)|(green<<8)|blue, period);
}

bool RGBClass::breath(uint32_t rgb, uint16_t period) {
    if(!_control) {
        return false;
    }

    HAL_UI_RGB_Breath(rgb, period);
    return true;
}

bool RGBClass::breath(uint8_t red, uint8_t green, uint8_t blue, uint16_t period) {
    return breath((red<<16)|(green<<8)|blue, period);
}

RGBClass RGB;
