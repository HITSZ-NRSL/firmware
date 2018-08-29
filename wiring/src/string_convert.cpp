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


#include "string_convert.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include <math.h>
#include "molmc_log.h"

//------------------------------------------------------------------------------------------
void str_reverse(char* begin, char* end)
{
    char *is = begin;
    char *ie = end - 1;
    while(is < ie) {
        char tmp = *ie;
        *ie = *is;
        *is = tmp;
        ++is;
        --ie;
    }
}

char* itoa(int value, char* result, int base)
{
    if(base < 2 || base > 16) {
        *result = 0;
        return result;
    }

    char* out = result;
    int quotient = abs(value);

    do {
        const int tmp = quotient / base;
        *out = "0123456789abcdef"[quotient - (tmp * base)];
        ++out;
        quotient = tmp;
    } while(quotient);

    // Apply negative sign
    if(value < 0)
        *out++ = '-';

    str_reverse(result, out);
    *out = 0;
    return result;
}

char* ltoa(long value, char* result, int base)
{
    if(base < 2 || base > 16) {
        *result = 0;
        return result;
    }

    char* out = result;
    long quotient = abs(value);

    do {
        const long tmp = quotient / base;
        *out = "0123456789abcdef"[quotient - (tmp * base)];
        ++out;
        quotient = tmp;
    } while(quotient);

    // Apply negative sign
    if(value < 0)
        *out++ = '-';

    str_reverse(result, out);
    *out = 0;
    return result;
}

char* utoa(unsigned value, char* result, int base)
{
    if(base < 2 || base > 16) {
        *result = 0;
        return result;
    }

    char* out = result;
    unsigned quotient = value;

    do {
        const unsigned tmp = quotient / base;
        *out = "0123456789abcdef"[quotient - (tmp * base)];
        ++out;
        quotient = tmp;
    } while(quotient);

    str_reverse(result, out);
    *out = 0;
    return result;
}

char* ultoa(unsigned long value, char* result, int base)
{
    if(base < 2 || base > 16) {
        *result = 0;
        return result;
    }

    char* out = result;
    unsigned long quotient = value;

    do {
        const unsigned long tmp = quotient / base;
        *out = "0123456789abcdef"[quotient - (tmp * base)];
        ++out;
        quotient = tmp;
    } while(quotient);

    str_reverse(result, out);
    *out = 0;
    return result;
}

char * dtostrf(double number, signed char width, unsigned char prec, char *s)
{
    bool negative = false;

    if (isnan(number)) {
        strcpy(s, "nan");
        return s;
    }
    if (isinf(number)) {
        strcpy(s, "inf");
        return s;
    }

    char* out = s;

    int fillme = width; // how many cells to fill for the integer part
    if (prec > 0) {
        fillme -= (prec+1);
    }

    // Handle negative numbers
    if (number < 0.0) {
        negative = true;
        fillme--;
        number = -number;
    }

    // Round correctly so that print(1.999, 2) prints as "2.00"
    // I optimized out most of the divisions
    double rounding = 2.0;
    for (uint8_t i = 0; i < prec; ++i)
        rounding *= 10.0;
    rounding = 1.0 / rounding;

    number += rounding;

    // Figure out how big our number really is
    double tenpow = 1.0;
    int digitcount = 1;
    while (number >= 10.0 * tenpow) {
        tenpow *= 10.0;
        digitcount++;
    }

    number /= tenpow;
    fillme -= digitcount;

    // Pad unused cells with spaces
    while (fillme-- > 0) {
        *out++ = ' ';
    }

    // Handle negative sign
    if (negative) *out++ = '-';

    // Print the digits, and if necessary, the decimal point
    digitcount += prec;
    int8_t digit = 0;
    while (digitcount-- > 0) {
        digit = (int8_t)number;
        if (digit > 9) digit = 9; // insurance
        *out++ = (char)('0' | digit);
        if ((digitcount == prec) && (prec > 0)) {
            *out++ = '.';
        }
        number -= digit;
        number *= 10.0;
    }

    // make sure the string is terminated
    *out = 0;
    return s;
}

int mac_str_to_bin( char *str, unsigned char *mac)
{
    int i;
    char *s, *e;

    if ((mac == NULL) || (str == NULL)) {
        return -1;
    }

    s = (char *) str;
    for (i = 0; i < 6; ++i) {
        mac[i] = s ? strtoul (s, &e, 16) : 0;
        if (s)
           s = (*e) ? e + 1 : e;
    }
    return 0;
}

static inline char ascii_nibble(uint8_t nibble)
{
    char hex_digit = nibble + 48;
    if (57 < hex_digit)
        hex_digit += 39;
    return hex_digit;
}

static inline uint8_t hex_nibble(uint8_t nibble)
{
    if(('0' <= nibble) && (nibble <= '9')) {
        nibble -= '0';
    } else if(('a' <= nibble) && (nibble <= 'f')) {
        nibble -= 'a';
        nibble += 10;
    } else if(('A' <= nibble) && (nibble <= 'F')) {
        nibble -= 'A';
        nibble += 10;
    } else {
        nibble = 0;
    }
    return nibble;
}

uint16_t string2hex(char *src, uint8_t *dest, uint16_t destlen, bool reverse)
{
    uint16_t srclen = strlen(src);
    int index = 0, n = 0;

    if(srclen%2) {
        srclen += 1;
    }

    if(false == reverse) {
        for(index = 0, n = 0; (index < srclen)&&(n <= destlen); index += 2) {
            dest[n++] = hex_nibble(src[index]) << 4 | hex_nibble(src[index+1]);
        }
    } else {
        for(index = srclen-2, n = 0; (index >= 0)&&(n <= destlen); index -= 2) {
            dest[n++] = hex_nibble(src[index]) << 4 | hex_nibble(src[index+1]);
        }
    }
    return n;
}

char *hex2string(uint8_t *src, uint16_t srclen, char *dest, bool reverse)
{
    int index = 0, n = 0;

    char* result = dest;
    if(false == reverse) {
        for(index = 0, n = 0; index < srclen; index++) {
            dest[n++] = ascii_nibble(src[index] >> 4);
            dest[n++] = ascii_nibble(src[index] & 0xF);
        }
    } else {
        for(index = srclen-1, n = 0; index >= 0; index--) {
            dest[n++] = ascii_nibble(src[index] >> 4);
            dest[n++] = ascii_nibble(src[index] & 0xF);
        }
    }
    return result;
}

//------------------------------------------------------------------------------------------
