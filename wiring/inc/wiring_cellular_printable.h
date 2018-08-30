/**
 ******************************************************************************
  Copyright (c) 2015 IntoRobot Industries, Inc.  All rights reserved.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#ifndef WIRING_CELLULAR_PRINTABLE_H_
#define WIRING_CELLULAR_PRINTABLE_H_

#include "intorobot_config.h"

#ifdef configWIRING_CELLULAR_ENABLE

#include "wiring_printable.h"
#include "wiring_string.h"
#include <string.h>
#include "cellular_hal.h"
#include "modem/inc/enums_hal.h"

/*
 * CellularSignal
 */
class CellularSignal : public Printable {
public:
    int rssi = 0;
    int qual = 0;

    CellularSignal() { /* n/a */ }

    virtual size_t printTo(Print& p) const;
};

#endif // Wiring_Cellular

#endif // WIRING_CELLULAR_PRINTABLE_H_
