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

#include "wiring_i2s.h"
#include "core_hal.h"
#include "intorobot_macros.h"

#define AUDIO_FREQ 16000
I2SClass::I2SClass(HAL_I2S_Interface i2s)
{
    _i2s = i2s;
    HAL_I2S_Initial(_i2s);
}

void I2SClass::begin()
{
    HAL_I2S_Begin(_i2s, AUDIO_FREQ);
}

void I2SClass::begin(uint32_t audioFreq)
{
    HAL_I2S_Begin(_i2s, audioFreq);
}

void I2SClass::end()
{
    HAL_I2S_End(_i2s);
}

uint16_t I2SClass::receive()
{
    return HAL_I2S_Receive_Data(_i2s);
}

bool I2SClass::isEnabled()
{
    return HAL_I2S_Is_Enabled(_i2s);
}


// 不要改成类 为了保证类构造函数使用时，已经初始化
#ifdef configWIRING_I2S_ENABLE
I2SClass& __fetch_global_i2s()
{
    static I2SClass i2s(HAL_I2S_INTERFACE1);
    return i2s;
}

#ifdef configWIRING_I2S1_ENABLE
I2SClass& __fetch_global_i2s1()
{
    static I2SClass i2s1(HAL_I2S_INTERFACE2);
    return i2s1;
}
#endif  // configWIRING_I2S1_ENABLE

#endif  // configWIRING_I2S_ENABLE

