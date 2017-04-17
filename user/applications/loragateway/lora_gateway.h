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

#ifndef LORA_GATEWAY_H_
#define LORA_GATEWAY_H_


// Definition of the configuration record that is read at startup and written
// when settings are changed.
struct LoraGwayConfig {
    uint8_t ch;                 // index to freqs array, freqs[ifreq]=868100000 default
    uint16_t fcnt;              // =0 as init value
    uint8_t sf;                 // range from SF7 to SF12
    uint8_t debug;              // range 0 to 4
    bool cad;
    bool hop;
};

#endif
