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

#include "wiring_usbserial.h"

#ifdef configWIRING_USBSERIAL_ENABLE


//
// Constructor
//
USBSerial::USBSerial()
{
    _blocking = true;
}

//
// Public methods
//

void USBSerial::begin(long speed)
{
    USB_USART_Initial((unsigned)speed);
}

void USBSerial::end()
{
    USB_USART_Initial(0);
}

// Read data from buffer
int USBSerial::read()
{
    return USB_USART_Receive_Data(false);
}

int USBSerial::availableForWrite()
{
    return USB_USART_Available_Data_For_Write();
}

int USBSerial::available()
{
    return USB_USART_Available_Data();
}

size_t USBSerial::write(uint8_t byte)
{
    if (USB_USART_Available_Data_For_Write() > 0 || _blocking) {
        USB_USART_Send_Data(byte);
        return 1;
    }
    return 0;
}

void USBSerial::flush()
{
    USB_USART_Flush_Data();
}

void USBSerial::blockOnOverrun(bool block)
{
    _blocking = block;
}

int USBSerial::peek()
{
    return USB_USART_Receive_Data(true);
}

// 不要改成类 为了保证类构造函数使用时，已经初始化
USBSerial& _fetch_global_usbserial()
{
    static USBSerial _globalUSBSerial;
    return _globalUSBSerial;
}

#endif
