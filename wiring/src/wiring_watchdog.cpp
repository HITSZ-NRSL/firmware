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

#include "wiring_watchdog.h"

#if 0
//#if PLATFORM_THREADING

volatile system_tick_t ApplicationWatchdog::last_checkin;

os_thread_return_t ApplicationWatchdog::start(void* pointer)
{
    ApplicationWatchdog& wd = *(ApplicationWatchdog*)pointer;
    wd.loop();
    os_thread_cleanup(nullptr);
}

void ApplicationWatchdog::loop()
{
    bool done = false;
    system_tick_t now;
    while (!done) {
        HAL_Delay_Milliseconds(timeout);
        now = current_time();
        done = (now-last_checkin)>=timeout;
    }

    if (timeout>0 && timeout_fn) {
        timeout_fn();
        timeout_fn = std::function<void(void)>();
    }
}


#endif
