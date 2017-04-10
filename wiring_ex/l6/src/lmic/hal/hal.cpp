/*******************************************************************************
 * Copyright (c) 2015 Matthijs Kooijman
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * This the HAL to run LMIC on top of the Arduino environment.
 *******************************************************************************/

#include "wiring.h"
#include "wiring_spi.h"
#include "wiring_interrupts.h"
#include "wiring_system.h"
#include "lmic/lmic.h"
#include "hal.h"

SPIClass SPI1(HAL_SPI_INTERFACE2);
// -----------------------------------------------------------------------------
// irq
static void hal_dio0_irq_handler () {
    radio_irq_handler(0);
}

static void hal_dio1_irq_handler () {
    radio_irq_handler(1);
}

static void hal_dio2_irq_handler () {
    radio_irq_handler(2);
}

// I/O
static void hal_io_init () {
    pinMode(SX1278_NSS, OUTPUT);
    pinMode(SX1278_RXTX, OUTPUT);
    pinMode(SX1278_RESET, OUTPUT);

    pinMode(SX1278_DIO0, INPUT);
    pinMode(SX1278_DIO1, INPUT);
    pinMode(SX1278_DIO2, INPUT);

    attachInterrupt(SX1278_DIO0, hal_dio0_irq_handler, RISING);
    attachInterrupt(SX1278_DIO1, hal_dio1_irq_handler, RISING);
    attachInterrupt(SX1278_DIO2, hal_dio2_irq_handler, RISING);
}

// val == 1  => tx 1
void hal_pin_rxtx (u1_t val) {
    digitalWrite(SX1278_RXTX, val);
}

// set radio RST pin to given value (or keep floating!)
void hal_pin_rst (u1_t val) {
    if(val == 0 || val == 1) { // drive pin
        pinMode(SX1278_RESET, OUTPUT);
        digitalWrite(SX1278_RESET, val);
    } else { // keep pin floating
        pinMode(SX1278_RESET, INPUT);
    }
}

// -----------------------------------------------------------------------------
// SPI
static const SPISettings settings(10E6, MSBFIRST, SPI_MODE0);

static void hal_spi_init () {
    SPI1.setBitOrder(MSBFIRST);
    SPI1.setClockDivider(SPI_CLOCK_DIV16);
    SPI1.setDataMode(0);
    SPI1.begin();
}

void hal_pin_nss (u1_t val) {
    /*
    if (!val)
        SPI1.beginTransaction(settings);
    else
        SPI1.endTransaction();
        */
    if (val)
        SPI1.endTransaction();

    //Serial.println(val?">>":"<<");
    digitalWrite(SX1278_NSS, val);
}

// perform SPI transaction with radio
u1_t hal_spi (u1_t out) {
    u1_t res = SPI1.transfer(out);
    /*
    Serial.print(">");
    Serial.print(out, HEX);
    Serial.print("<");
    Serial.println(res, HEX);
    */
    return res;
}

// -----------------------------------------------------------------------------
// TIME

static void hal_time_init () {
    // Nothing to do
}

u4_t hal_ticks () {
    // Because micros() is scaled down in this function, micros() will
    // overflow before the tick timer should, causing the tick timer to
    // miss a significant part of its values if not corrected. To fix
    // this, the "overflow" serves as an overflow area for the micros()
    // counter. It consists of three parts:
    //  - The US_PER_OSTICK upper bits are effectively an extension for
    //    the micros() counter and are added to the result of this
    //    function.
    //  - The next bit overlaps with the most significant bit of
    //    micros(). This is used to detect micros() overflows.
    //  - The remaining bits are always zero.
    //
    // By comparing the overlapping bit with the corresponding bit in
    // the micros() return value, overflows can be detected and the
    // upper bits are incremented. This is done using some clever
    // bitwise operations, to remove the need for comparisons and a
    // jumps, which should result in efficient code. By avoiding shifts
    // other than by multiples of 8 as much as possible, this is also
    // efficient on AVR (which only has 1-bit shifts).
    static uint8_t overflow = 0;

    // Scaled down timestamp. The top US_PER_OSTICK_EXPONENT bits are 0,
    // the others will be the lower bits of our return value.
    uint32_t scaled = micros() >> US_PER_OSTICK_EXPONENT;
    // Most significant byte of scaled
    uint8_t msb = scaled >> 24;
    // Mask pointing to the overlapping bit in msb and overflow.
    const uint8_t mask = (1 << (7 - US_PER_OSTICK_EXPONENT));
    // Update overflow. If the overlapping bit is different
    // between overflow and msb, it is added to the stored value,
    // so the overlapping bit becomes equal again and, if it changed
    // from 1 to 0, the upper bits are incremented.
    overflow += (msb ^ overflow) & mask;

    // Return the scaled value with the upper bits of stored added. The
    // overlapping bit will be equal and the lower bits will be 0, so
    // bitwise or is a no-op for them.
    return scaled | ((uint32_t)overflow << 24);

    // 0 leads to correct, but overly complex code (it could just return
    // micros() unmodified), 8 leaves no room for the overlapping bit.
    static_assert(US_PER_OSTICK_EXPONENT > 0 && US_PER_OSTICK_EXPONENT < 8, "Invalid US_PER_OSTICK_EXPONENT value");
}

// Returns the number of ticks until time. Negative values indicate that
// time has already passed.
static s4_t delta_time(u4_t time) {
    return (s4_t)(time - hal_ticks());
}

void hal_waitUntil (u4_t time) {
    s4_t delta = delta_time(time);
    // From delayMicroseconds docs: Currently, the largest value that
    // will produce an accurate delay is 16383.
    while (delta > (16000 / US_PER_OSTICK)) {
        delay(16);
        delta -= (16000 / US_PER_OSTICK);
    }
    if (delta > 0)
        delayMicroseconds(delta * US_PER_OSTICK);
}

// check and rewind for target time
u1_t hal_checkTimer (u4_t time) {
    // No need to schedule wakeup, since we're not sleeping
    return delta_time(time) <= 0;
}

static uint8_t irqlevel = 0;

void hal_disableIRQs () {
    noInterrupts();
    irqlevel++;
}

void hal_enableIRQs () {
    if(--irqlevel == 0) {
        interrupts();
    }
}

void hal_sleep () {
    // Not implemented
}

// -----------------------------------------------------------------------------
void hal_init () {
    hal_disableIRQs();
    // configure radio I/O and interrupt handler
    hal_io_init();
    // configure radio SPI
    hal_spi_init();
    // configure timer and interrupt handler
    hal_time_init();

    hal_enableIRQs();
}

void hal_failed (const char *file, u2_t line) {
#if defined(LMIC_FAILURE_TO)
    LMIC_FAILURE_TO.println("FAILURE ");
    LMIC_FAILURE_TO.print(file);
    LMIC_FAILURE_TO.print(':');
    LMIC_FAILURE_TO.println(line);
    LMIC_FAILURE_TO.flush();
#endif
    hal_disableIRQs();
    hal_sleep();
    while(1);
}
