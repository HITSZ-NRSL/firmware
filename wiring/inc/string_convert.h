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

#ifndef STRING_CONVERT_H_
#define STRING_CONVERT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

char* itoa (int val, char *s, int radix);
char* ltoa (long val, char *s, int radix);
char* utoa (unsigned int val, char *s, int radix);
char* ultoa (unsigned long val, char *s, int radix);
char* dtostrf (double val, signed char width, unsigned char prec, char *s);
int mac_str_to_bin(char *str, unsigned char *mac);

uint16_t string2hex(char *src, uint8_t *dest, uint16_t destlen, bool reverse);
char *hex2string(uint8_t *src, uint16_t srclen, char *dest, bool reverse);

#ifdef __cplusplus
}
#endif

#endif	/* STRING_CONVERT_H */

