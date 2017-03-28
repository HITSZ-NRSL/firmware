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

#ifndef WIRING_I2S_H_
#define WIRING_I2S_H_

#include "wiring.h"
#include "i2s_hal.h"

class I2SClass {
    private:
        HAL_I2S_Interface _i2s;

    public:
        I2SClass(HAL_I2S_Interface i2s);
        virtual ~I2SClass() {};

        void begin();
        void begin(uint32_t audioFreq);
        void end();

        uint16_t receive();
        bool isEnabled();
};

// 不要改成类 为了保证类构造函数使用时，已经初始化
#ifdef configWIRING_I2S_ENABLE
#ifdef I2S
#undef I2S
#endif

#define I2S __fetch_global_i2s()
I2SClass& __fetch_global_i2s();

#ifdef configWIRING_I2S1_ENABLE
#ifdef I2S1
#undef I2S1
#endif
#define I2S1 __fetch_global_i2s1()
I2SClass& __fetch_global_i2s1();
#endif  // configWIRING_I2S1_ENABLE

#endif  // configWIRING_I2S_ENABLE

#endif
