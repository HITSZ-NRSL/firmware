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
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    size_t size;
    void (*notify_connected)();     // HAL_NET_notify_connected()
    void (*notify_disconnected)();  // HAL_NET_notify_disconnected()
    void (*notify_dhcp)(bool dhcp); // HAL_NET_notify_dhcp()
    void (*notify_can_shutdown)();  // HAL_NET_notify_can_shutdown()
} HAL_NET_Callbacks;

/**
 * Sets notification callbacks. This function is used when HAL implementation cannot be linked
 * with HAL_NET_notify_*() functions directly.
 */
void HAL_NET_SetCallbacks(const HAL_NET_Callbacks* callbacks, void* reserved);

void HAL_NET_notify_connected();
void HAL_NET_notify_disconnected();
void HAL_NET_notify_dhcp(bool dhcp);
void HAL_NET_notify_can_shutdown();

uint32_t HAL_NET_SetNetWatchDog(uint32_t timeOutInuS);

#ifdef __cplusplus
}
#endif
