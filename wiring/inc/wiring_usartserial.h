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

#ifndef WIRING_USARTSERIAL_H_
#define WIRING_USARTSERIAL_H_

#include "intorobot_config.h"
#include "wiring_stream.h"
#include "usart_hal.h"

class USARTSerial : public Stream
{
    private:
        HAL_USART_Serial _serial;
        bool _blocking;
    public:
        USARTSerial(HAL_USART_Serial serial);
        virtual ~USARTSerial() {};
        void begin(unsigned long);
        void begin(unsigned long, uint8_t);
        void halfduplex(bool);
        void end();

        virtual void blockOnOverrun(bool);

        virtual int availableForWrite(void);
        virtual int available(void);
        virtual int peek(void);
        virtual int read(void);
        virtual void flush(void);
        size_t write(uint16_t);
        virtual size_t write(uint8_t);

        inline size_t write(unsigned long n) { return write((uint8_t)n); }
        inline size_t write(long n) { return write((uint8_t)n); }
        inline size_t write(unsigned int n) { return write((uint8_t)n); }
        inline size_t write(int n) { return write((uint8_t)n); }

        using Print::write; // pull in write(str) and write(buf, size) from Print

        operator bool();

        bool isEnabled(void);
};

#ifdef configWIRING_USARTSERIAL1_ENABLE
void serialEventRun1(void) __attribute__((weak));
void serialEvent1(void) __attribute__((weak));
#endif

#ifdef configWIRING_USARTSERIAL2_ENABLE
void serialEventRun2(void) __attribute__((weak));
void serialEvent2(void) __attribute__((weak));
#endif

#ifdef configWIRING_USARTSERIAL3_ENABLE
void serialEventRun3(void) __attribute__((weak));
void serialEvent3(void) __attribute__((weak));
#endif

#ifdef configWIRING_USARTSERIAL4_ENABLE
void serialEventRun4(void) __attribute__((weak));
void serialEvent4(void) __attribute__((weak));
#endif

#ifdef configWIRING_USARTSERIAL5_ENABLE
void serialEventRun5(void) __attribute__((weak));
void serialEvent5(void) __attribute__((weak));
#endif

inline void __handleSerialEvent(USARTSerial& serial, void (*handler)(void)) __attribute__((always_inline));

inline void __handleSerialEvent(USARTSerial& serial, void (*handler)(void))
{
    if (handler && serial.isEnabled() && serial.available()>0)
        handler();
}

// 不要改成类 为了保证类构造函数使用时，已经初始化
#ifdef configWIRING_USARTSERIAL_ENABLE
extern USARTSerial &__fetch_global_serial();
#define Serial __fetch_global_serial()

#ifdef configWIRING_USARTSERIAL1_ENABLE
extern USARTSerial &__fetch_global_serial1();
#define Serial1 __fetch_global_serial1()
#endif

#ifdef configWIRING_USARTSERIAL2_ENABLE
extern USARTSerial &__fetch_global_serial2();
#define Serial2 __fetch_global_serial2()
#endif

#endif

#endif
