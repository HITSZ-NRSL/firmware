#if 0
// pull in the sources from the HAL. It's a bit of a hack, but is simpler than trying to link the
// full hal library.
#include "../src/esp8266_share/esp8266/esp8266_neopixel.c"

#else

#include "hw_config.h"
#include "eagle_soc.h"

#define xt_rsil(level) (__extension__({uint32_t state; __asm__ __volatile__("rsil %0," __STRINGIFY(level) : "=a" (state)); state;}))
#define _BV(b) (1UL << (b))

static uint32_t _getCycleCount(void) __attribute__((always_inline));
static inline uint32_t _getCycleCount(void) {
    uint32_t ccount;
    __asm__ __volatile__("rsr %0,ccount":"=a" (ccount));
    return ccount;
}

void ICACHE_RAM_ATTR espShow(uint8_t pin, uint32_t color, uint8_t brightness) {
//void espShow(uint8_t pin, uint32_t color, uint8_t brightness) {
#define CYCLES_800_T0H  (F_CPU / 2500000) // 0.4us
#define CYCLES_800_T1H  (F_CPU / 1250000) // 0.8us
#define CYCLES_800      (F_CPU /  800000) // 1.25us per bit

    uint8_t *p, *end, pix, mask, ctmp[3];
    uint32_t t, time0, time1, period, c, startTime, pinMask;

    ctmp[1] = ((uint8_t)(color >> 16) * brightness)>>8;
    ctmp[0] = ((uint8_t)(color >> 8) * brightness)>>8;
    ctmp[2] = ((uint8_t)color * brightness)>>8;

    pinMask   = _BV(pin);
    p         =  ctmp;
    end       =  p + 3;
    pix       = *p++;
    mask      = 0x80;
    startTime = 0;

    time0  = CYCLES_800_T0H;
    time1  = CYCLES_800_T1H;
    period = CYCLES_800;

    xt_rsil(15);
    for(t = time0;; t = time0) {
        if(pix & mask) t = time1;                             // Bit high duration
        while(((c = _getCycleCount()) - startTime) < period); // Wait for bit start
        GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, pinMask);       // Set high
        startTime = c;                                        // Save start time
        while(((c = _getCycleCount()) - startTime) < t);      // Wait high duration
        GPOC = (1 << pin);
        GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, pinMask);       // Set low
        if(!(mask >>= 1)) {                                   // Next bit/byte
            if(p >= end) break;
            pix  = *p++;
            mask = 0x80;
        }
    }
    while((_getCycleCount() - startTime) < period); // Wait for last bit
    xt_rsil(0);
}

#endif
