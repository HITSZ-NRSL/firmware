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
#include "lora_gateway_mgt.h"

// ----------------------------------------------------------------------------
// Ths function gateway_mgt is called in the UDP Receive function after
// all well-known LoRa Gateway messages are scanned.
//
// As part of this function we will listen for another set of messages
// that is defined in loraModem.h.
// All opCodes start with 0x1y for at leaving opcodes 0x00 to 0x0F to the
// pure Gateway protocol
//
// Incoming mesage format:
// buf[0]-buf[2], These are 0x00 or dont care
// buf[3], contains opcode
// buf[4]-buf[7], contains parameter max. 4 bytes.
//
// Upstream Message format:
//
// ----------------------------------------------------------------------------
void gateway_mgt(uint8_t size, uint8_t *buff) {

    uint8_t opcode = buff[3];

    switch (opcode) {
        case MGT_RESET:
            Serial.println(F("gateway_mgt:: RESET"));
            // No further parameters, just reset the GWay
            setup();        // Call the sketch setup function
            // Send Ack to server

            break;
        case MGT_SET_SF:
            Serial.println(F("gateway_mgt:: SET SF"));
            // byte [4] contains desired SF code (7 for SF7 and 12 for SF12)
            break;
        case MGT_SET_FREQ:
            Serial.println(F("gateway_mgt:: SET FREQ"));
            // Byte [4] contains index of Frequency
            break;
        default:
            Serial.print(F("gateway_mgt:: Unknown UDP code="));
            Serial.println(opcode);
            return;
            break;
    }
}

