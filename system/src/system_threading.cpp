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

#if PLATFORM_THREADING

#include "system_threading.h"
#include "system_task.h"
#include "FreeRTOS.h"
#include "task.h"

static TaskHandle_t  system_thread_handle;
#define SYSTEM_TREAD_STACK_SIZE         6144

static void system_task_start(void *argument)
{
    system_process_loop();
}

void create_system_task(void)
{
    /*system_tread*/
    os_thread_create(&system_thread_handle, "system_thread", OS_THREAD_PRIORITY_DEFAULT, system_task_start, NULL, SYSTEM_TREAD_STACK_SIZE);
}

void close_system_task(void)
{
    os_thread_cleanup(system_thread_handle);
}

static os_mutex_recursive_t usb_serial_mutex;

os_mutex_recursive_t mutex_usb_serial()
{
    if (nullptr==usb_serial_mutex) {
        os_mutex_recursive_create(&usb_serial_mutex);
    }
    return usb_serial_mutex;
}

void* system_internal(int item, void* reserved)
{
    switch (item) {
#if PLATFORM_THREADING
    //case 0: return &ApplicationThread;
    //case 1: return &SystemThread;
    case 2: return mutex_usb_serial();
#endif
    }
    return nullptr;
}

#endif
