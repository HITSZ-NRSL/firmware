/*
   SPI.cpp - SPI library for esp8266

   Copyright (c) 2015 Hristo Gochkov. All rights reserved.
   This file is part of the esp8266 core for Arduino environment.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
   */

#include <string.h>
#include "pinmap_hal.h"
#include "esp8266_peri.h"
#include "esp8266-hal-spi.h"
#include "esp8266-hal-gpio.h"

#define SPI_PINS_HSPI			0 // Normal HSPI mode (MISO = GPIO12, MOSI = GPIO13, SCLK = GPIO14);
#define SPI_PINS_HSPI_OVERLAP	1 // HSPI Overllaped in spi0 pins (MISO = SD0, MOSI = SDD1, SCLK = CLK);

#define SPI_OVERLAP_SS 0


typedef union {
    uint32_t regValue;
    struct {
        unsigned regL :6;
        unsigned regH :6;
        unsigned regN :6;
        unsigned regPre :13;
        unsigned regEQU :1;
    };
} spiClk_t;

static bool useHwCs;
static uint8_t pinSet;
static pin_t sck_pin;
static pin_t mosi_pin;
static pin_t miso_pin;
static pin_t ss_pin;

//选择spi功能　但目前只支持HSPI
void spi_selectSet(void)
{
    useHwCs = false;
    pinSet = SPI_PINS_HSPI;
}

bool spi_pins(int8_t sck, int8_t miso, int8_t mosi, int8_t ss)
{
    if (sck == 6 &&
            miso == 7 &&
            mosi == 8 &&
            ss == 0) {
        pinSet = SPI_PINS_HSPI_OVERLAP;
    } else if (sck == 14 &&
            miso == 12 &&
            mosi == 13) {
        pinSet = SPI_PINS_HSPI;
    } else {
        return false;
    }

    return true;
}

void spi_begin(int8_t sck, int8_t miso, int8_t mosi, int8_t ss)
{
    sck_pin = sck;
    miso_pin = miso;
    mosi_pin = mosi;
    ss_pin = ss;

    switch (pinSet)
    {
        case SPI_PINS_HSPI_OVERLAP:
            IOSWAP |= (1 << IOSWAP2CS);
            //SPI0E3 |= 0x1; This is in the MP3_DECODER example, but makes the WD kick in here.
            SPI1E3 |= 0x3;

            spi_setHwCs(true);
            break;
        case SPI_PINS_HSPI:
        default:
            {
                __pinMode(sck_pin, ESP8266_SPECIAL);  ///< GPIO14
                __pinMode(miso_pin, ESP8266_SPECIAL); ///< GPIO12
                __pinMode(mosi_pin, ESP8266_SPECIAL); ///< GPIO13
            }
            break;
    }

    SPI1C = 0;
    spi_setFrequency(1000000); ///< 1MHz
    SPI1U = SPIUMOSI | SPIUDUPLEX | SPIUSSE;
    SPI1U1 = (7 << SPILMOSI) | (7 << SPILMISO);
    SPI1C1 = 0;
}

void spi_end() {
    switch (pinSet) {
        case SPI_PINS_HSPI:
            __pinMode(sck_pin, ESP8266_INPUT);
            __pinMode(miso_pin, ESP8266_INPUT);
            __pinMode(mosi_pin, ESP8266_INPUT);
            if (useHwCs) {
                __pinMode(ss_pin, ESP8266_INPUT);
            }
            break;
        case SPI_PINS_HSPI_OVERLAP:
            IOSWAP &= ~(1 << IOSWAP2CS);
            if (useHwCs) {
                SPI1P |= SPIPCS1DIS | SPIPCS0DIS | SPIPCS2DIS;
                __pinMode(SPI_OVERLAP_SS, ESP8266_INPUT);
            }
            break;
    }
}

void spi_setHwCs(bool use) {
    switch (pinSet) {
        case SPI_PINS_HSPI:
            if (use) {
                __pinMode(ss_pin, ESP8266_SPECIAL); ///< GPIO15
                SPI1U |= (SPIUCSSETUP | SPIUCSHOLD);
            } else {
                if (useHwCs) {
                    __pinMode(ss_pin, ESP8266_INPUT);
                    SPI1U &= ~(SPIUCSSETUP | SPIUCSHOLD);
                }
            }
            break;
        case SPI_PINS_HSPI_OVERLAP:
            if (use) {
                __pinMode(SPI_OVERLAP_SS, ESP8266_FUNCTION_1); // GPI0 to SPICS2 mode
                SPI1P &= ~SPIPCS2DIS;
                SPI1P |= SPIPCS1DIS | SPIPCS0DIS;
                SPI1U |= (SPIUCSSETUP | SPIUCSHOLD);
            }
            else {
                if (useHwCs) {
                    __pinMode(SPI_OVERLAP_SS, ESP8266_INPUT);
                    SPI1P |= SPIPCS1DIS | SPIPCS0DIS | SPIPCS2DIS;
                    SPI1U &= ~(SPIUCSSETUP | SPIUCSHOLD);
                }
            }
            break;
    }

    useHwCs = use;
}


void spi_endTransaction() {
}

void spi_setDataMode(uint8_t dataMode) {

    /**
      SPI_MODE0 0x00 - CPOL: 0  CPHA: 0
      SPI_MODE1 0x01 - CPOL: 0  CPHA: 1
      SPI_MODE2 0x10 - CPOL: 1  CPHA: 0
      SPI_MODE3 0x11 - CPOL: 1  CPHA: 1
      */

    bool CPOL = (dataMode & 0x10); ///< CPOL (Clock Polarity)
    bool CPHA = (dataMode & 0x01); ///< CPHA (Clock Phase)

    if(CPHA) {
        SPI1U |= (SPIUSME);
    } else {
        SPI1U &= ~(SPIUSME);
    }

    if(CPOL) {
        SPI1P |= 1<<29;
    } else {
        SPI1P &= ~(1<<29);
        //todo test whether it is correct to set CPOL like this.
    }
}

void spi_setBitOrder(uint8_t bitOrder) {
    if(bitOrder == MSBFIRST) {
        SPI1C &= ~(SPICWBO | SPICRBO);
    } else {
        SPI1C |= (SPICWBO | SPICRBO);
    }
}

/**
 * calculate the Frequency based on the register value
 * @param reg
 * @return
 */
static uint32_t ClkRegToFreq(spiClk_t * reg) {
    return (ESP8266_CLOCK / ((reg->regPre + 1) * (reg->regN + 1)));
}

void spi_setFrequency(uint32_t freq) {
    static uint32_t lastSetFrequency = 0;
    static uint32_t lastSetRegister = 0;

    if(freq >= ESP8266_CLOCK) {
        spi_setClockDivider(0x80000000);
        return;
    }

    if(lastSetFrequency == freq && lastSetRegister == SPI1CLK) {
        // do nothing (speed optimization)
        return;
    }

    const spiClk_t minFreqReg = { 0x7FFFF000 };
    uint32_t minFreq = ClkRegToFreq((spiClk_t*) &minFreqReg);
    if(freq < minFreq) {
        // use minimum possible clock
        spi_setClockDivider(minFreqReg.regValue);
        lastSetRegister = SPI1CLK;
        lastSetFrequency = freq;
        return;
    }

    uint8_t calN = 1;

    spiClk_t bestReg = { 0 };
    int32_t bestFreq = 0;

    // find the best match
    while(calN <= 0x3F) { // 0x3F max for N

        spiClk_t reg = { 0 };
        int32_t calFreq;
        int32_t calPre;
        int8_t calPreVari = -2;

        reg.regN = calN;

        while(calPreVari++ <= 1) { // test different variants for Pre (we calculate in int so we miss the decimals, testing is the easyest and fastest way)
            calPre = (((ESP8266_CLOCK / (reg.regN + 1)) / freq) - 1) + calPreVari;
            if(calPre > 0x1FFF) {
                reg.regPre = 0x1FFF; // 8191
            } else if(calPre <= 0) {
                reg.regPre = 0;
            } else {
                reg.regPre = calPre;
            }

            reg.regL = ((reg.regN + 1) / 2);
            // reg.regH = (reg.regN - reg.regL);

            // test calculation
            calFreq = ClkRegToFreq(&reg);
            //os_printf("-----[0x%08X][%d]\t EQU: %d\t Pre: %d\t N: %d\t H: %d\t L: %d = %d\n", reg.regValue, freq, reg.regEQU, reg.regPre, reg.regN, reg.regH, reg.regL, calFreq);

            if(calFreq == (int32_t) freq) {
                // accurate match use it!
                memcpy(&bestReg, &reg, sizeof(bestReg));
                break;
            } else if(calFreq < (int32_t) freq) {
                // never go over the requested frequency
                if(abs(freq - calFreq) < abs(freq - bestFreq)) {
                    bestFreq = calFreq;
                    memcpy(&bestReg, &reg, sizeof(bestReg));
                }
            }
        }
        if(calFreq == (int32_t) freq) {
            // accurate match use it!
            break;
        }
        calN++;
    }

    spi_setClockDivider(bestReg.regValue);
    lastSetRegister = SPI1CLK;
    lastSetFrequency = freq;
}

void spi_setClockDivider(uint32_t clockDiv) {
    if(clockDiv == 0x80000000) {
        GPMUX |= (1 << 9); // Set bit 9 if sysclock required
    } else {
        GPMUX &= ~(1 << 9);
    }
    SPI1CLK = clockDiv;
}

inline void spi_setDataBits(uint16_t bits) {
    const uint32_t mask = ~((SPIMMOSI << SPILMOSI) | (SPIMMISO << SPILMISO));
    bits--;
    SPI1U1 = ((SPI1U1 & mask) | ((bits << SPILMOSI) | (bits << SPILMISO)));
}

uint8_t spi_transfer(uint8_t data) {
    while(SPI1CMD & SPIBUSY) {}
    // reset to 8Bit mode
    spi_setDataBits(8);
    SPI1W0 = data;
    SPI1CMD |= SPIBUSY;
    while(SPI1CMD & SPIBUSY) {}
    return (uint8_t) (SPI1W0 & 0xff);
}

uint16_t spi_transfer16(uint16_t data) {
    union {
        uint16_t val;
        struct {
            uint8_t lsb;
            uint8_t msb;
        };
    } in, out;
    in.val = data;

    if((SPI1C & (SPICWBO | SPICRBO))) {
        //LSBFIRST
        out.lsb = transfer(in.lsb);
        out.msb = transfer(in.msb);
    } else {
        //MSBFIRST
        out.msb = transfer(in.msb);
        out.lsb = transfer(in.lsb);
    }
    return out.val;
}

void spi_write(uint8_t data) {
    while(SPI1CMD & SPIBUSY) {}
    // reset to 8Bit mode
    spi_setDataBits(8);
    SPI1W0 = data;
    SPI1CMD |= SPIBUSY;
    while(SPI1CMD & SPIBUSY) {}
}

void spi_write16(uint16_t data)
{
    spi_write16Msb(data, !(SPI1C & (SPICWBO | SPICRBO)));
}

void spi_write16Msb(uint16_t data, bool msb)
{
    while(SPI1CMD & SPIBUSY) {}
    // Set to 16Bits transfer
    spi_setDataBits(16);
    if(msb) {
        // MSBFIRST Byte first
        SPI1W0 = (data >> 8) | (data << 8);
        SPI1CMD |= SPIBUSY;
    } else {
        // LSBFIRST Byte first
        SPI1W0 = data;
        SPI1CMD |= SPIBUSY;
    }
    while(SPI1CMD & SPIBUSY) {}
}

void spi_write32(uint32_t data) {
    spi_write32Msb(data, !(SPI1C & (SPICWBO | SPICRBO)));
}

void spi_write32Msb(uint32_t data, bool msb) {
    while(SPI1CMD & SPIBUSY) {}
    // Set to 32Bits transfer
    spi_setDataBits(32);
    if(msb) {
        union {
            uint32_t l;
            uint8_t b[4];
        } data_;
        data_.l = data;
        // MSBFIRST Byte first
        SPI1W0 = (data_.b[3] | (data_.b[2] << 8) | (data_.b[1] << 16) | (data_.b[0] << 24));
        SPI1CMD |= SPIBUSY;
    } else {
        // LSBFIRST Byte first
        SPI1W0 = data;
        SPI1CMD |= SPIBUSY;
    }
    while(SPI1CMD & SPIBUSY) {}
}

/**
 * Note:
 *  data need to be aligned to 32Bit
 *  or you get an Fatal exception (9)
 * @param data uint8_t *
 * @param size uint32_t
 */
void spi_writeBytes(uint8_t * data, uint32_t size) {
    while(size) {
        if(size > 64) {
            spi_writeBytes_(data, 64);
            size -= 64;
            data += 64;
        } else {
            spi_writeBytes_(data, size);
            size = 0;
        }
    }
}

void spi_writeBytes_(uint8_t * data, uint8_t size) {
    while(SPI1CMD & SPIBUSY) {}
    // Set Bits to transfer
    spi_setDataBits(size * 8);

    volatile uint32_t * fifoPtr = &SPI1W0;
    uint32_t * dataPtr = (uint32_t*) data;
    uint8_t dataSize = ((size + 3) / 4);

    while(dataSize--) {
        *fifoPtr = *dataPtr;
        dataPtr++;
        fifoPtr++;
    }

    SPI1CMD |= SPIBUSY;
    while(SPI1CMD & SPIBUSY) {}
}

/**
 * @param data uint8_t *
 * @param size uint8_t  max for size is 64Byte
 * @param repeat uint32_t
 */
void spi_writePattern(uint8_t * data, uint8_t size, uint32_t repeat) {
    if(size > 64) return; //max Hardware FIFO

    while(SPI1CMD & SPIBUSY) {}

    uint32_t buffer[16];
    uint8_t *bufferPtr=(uint8_t *)&buffer;
    uint8_t *dataPtr = data;
    volatile uint32_t * fifoPtr = &SPI1W0;
    uint8_t r;
    uint32_t repeatRem;
    uint8_t i;

    if((repeat * size) <= 64){
        repeatRem = repeat * size;
        r = repeat;
        while(r--){
            dataPtr = data;
            for(i=0; i<size; i++){
                *bufferPtr = *dataPtr;
                bufferPtr++;
                dataPtr++;
            }
        }

        r = repeatRem;
        if(r & 3) r = r / 4 + 1;
        else r = r / 4;
        for(i=0; i<r; i++){
            *fifoPtr = buffer[i];
            fifoPtr++;
        }
        SPI1U = SPIUMOSI | SPIUSSE;
    } else {
        //Orig
        r = 64 / size;
        repeatRem = repeat % r * size;
        repeat = repeat / r;

        while(r--){
            dataPtr = data;
            for(i=0; i<size; i++){
                *bufferPtr = *dataPtr;
                bufferPtr++;
                dataPtr++;
            }
        }

        //Fill fifo with data
        for(i=0; i<16; i++){
            *fifoPtr = buffer[i];
            fifoPtr++;
        }

        r = 64 / size;

        SPI1U = SPIUMOSI | SPIUSSE;
        spi_setDataBits(r * size * 8);
        while(repeat--){
            SPI1CMD |= SPIBUSY;
            while(SPI1CMD & SPIBUSY) {}
        }
    }
    //End orig
    spi_setDataBits(repeatRem * 8);
    SPI1CMD |= SPIBUSY;
    while(SPI1CMD & SPIBUSY) {}

    SPI1U = SPIUMOSI | SPIUDUPLEX | SPIUSSE;
}

/**
 * Note:
 *  in and out need to be aligned to 32Bit
 *  or you get an Fatal exception (9)
 * @param out uint8_t *
 * @param in  uint8_t *
 * @param size uint32_t
 */
void spi_transferBytes(uint8_t * out, uint8_t * in, uint32_t size) {
    while(size) {
        if(size > 64) {
            spi_transferBytes_(out, in, 64);
            size -= 64;
            if(out) out += 64;
            if(in) in += 64;
        } else {
            spi_transferBytes_(out, in, size);
            size = 0;
        }
    }
}

void spi_transferBytes_(uint8_t * out, uint8_t * in, uint8_t size) {
    while(SPI1CMD & SPIBUSY) {}
    // Set in/out Bits to transfer

    spi_setDataBits(size * 8);

    volatile uint32_t * fifoPtr = &SPI1W0;
    uint8_t dataSize = ((size + 3) / 4);

    if(out) {
        uint32_t * dataPtr = (uint32_t*) out;
        while(dataSize--) {
            *fifoPtr = *dataPtr;
            dataPtr++;
            fifoPtr++;
        }
    } else {
        // no out data only read fill with dummy data!
        while(dataSize--) {
            *fifoPtr = 0xFFFFFFFF;
            fifoPtr++;
        }
    }

    SPI1CMD |= SPIBUSY;
    while(SPI1CMD & SPIBUSY) {}

    if(in) {
        volatile uint8_t * fifoPtr8 = (volatile uint8_t *) &SPI1W0;
        dataSize = size;
        while(dataSize--) {
            *in = *fifoPtr8;
            in++;
            fifoPtr8++;
        }
    }
}

