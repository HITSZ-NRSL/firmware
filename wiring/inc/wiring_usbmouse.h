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

#ifndef WIRING_USBMOUSE_H_
#define WIRING_USBMOUSE_H_

#include "firmware_config.h"

#ifdef FIRMWARE_CONFIG_WIRING_USBMOUSE
#include "wiring.h"

#define MOUSE_LEFT		0x01
#define MOUSE_RIGHT		0x02
#define MOUSE_MIDDLE	0x04
#define MOUSE_ALL		(MOUSE_LEFT | MOUSE_RIGHT | MOUSE_MIDDLE)

typedef struct
{
	uint8_t buttons;
	int8_t x;
	int8_t y;
	int8_t wheel;
} MouseReport;

class USBMouse
{
private:
	MouseReport mouseReport;
	void buttons(uint8_t button);

public:
	USBMouse(void);

	void begin(void);
	void end(void);
	void move(int8_t x, int8_t y, int8_t wheel);
	void click(uint8_t button = MOUSE_LEFT);
	void press(uint8_t button = MOUSE_LEFT);		// press LEFT by default
	void release(uint8_t button = MOUSE_LEFT);		// release LEFT by default
	bool isPressed(uint8_t button = MOUSE_LEFT);	// check LEFT by default
};

extern USBMouse Mouse;
#endif

#endif
