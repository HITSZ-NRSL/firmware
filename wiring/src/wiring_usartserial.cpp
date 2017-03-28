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

#include "wiring_usartserial.h"
#include "wiring_constants.h"

// Constructors ////////////////////////////////////////////////////////////////

USARTSerial::USARTSerial(HAL_USART_Serial serial)
{
    _serial = serial;
    // Default is blocking mode
    _blocking = true;
    HAL_USART_Initial(serial);
}
// Public Methods //////////////////////////////////////////////////////////////

void USARTSerial::begin(unsigned long baud)
{
    begin(baud, SERIAL_8N1);
}

void USARTSerial::begin(unsigned long baud, byte config)
{
    HAL_USART_BeginConfig(_serial, baud, config, 0);
}

void USARTSerial::end()
{
    HAL_USART_End(_serial);
}

void USARTSerial::halfduplex(bool Enable)
{
    HAL_USART_Half_Duplex(_serial, Enable);
}

void USARTSerial::blockOnOverrun(bool block)
{
    _blocking = block;
}


int USARTSerial::availableForWrite(void)
{
    return HAL_USART_Available_Data(_serial);
}

int USARTSerial::available(void)
{
    return HAL_USART_Available_Data(_serial);
}

int USARTSerial::peek(void)
{
    return HAL_USART_Peek_Data(_serial);
}

int USARTSerial::read(void)
{
    return HAL_USART_Read_Data(_serial);
}

void USARTSerial::flush()
{
    HAL_USART_Flush_Data(_serial);
}

size_t USARTSerial::write(uint8_t c)
{
    // attempt a write if blocking, or for non-blocking if there is room.
    if (_blocking || HAL_USART_Available_Data_For_Write(_serial) > 0) {
        // the HAL always blocks.
        return HAL_USART_Write_Data(_serial, c);
    }
    return 0;
}

size_t USARTSerial::write(uint16_t c)
{
    return HAL_USART_Write_NineBitData(_serial, c);
}

USARTSerial::operator bool() {
    return true;
}

bool USARTSerial::isEnabled() {
    return HAL_USART_Is_Enabled(_serial);
}



// 不要改成类 为了保证类构造函数使用时，已经初始化
#ifdef configWIRING_USARTSERIAL_ENABLE
USARTSerial &__fetch_global_serial()
{
    static USARTSerial serial(HAL_USART_SERIAL1);
    return serial;
}

#ifdef configWIRING_USARTSERIAL1_ENABLE
USARTSerial &__fetch_global_serial1()
{
    static USARTSerial serial1(HAL_USART_SERIAL2);
    return serial1;
}
#endif

#ifdef configWIRING_USARTSERIAL2_ENABLE
USARTSerial &__fetch_global_serial2()
{
    static USARTSerial serial2(HAL_USART_SERIAL3);
    return serial2;
}
#endif

#endif

