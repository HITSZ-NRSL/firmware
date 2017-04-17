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

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include "config.h"
#include "intorobot_macros.h"
#include "service_debug.h"
#include "timer_hal.h"

#undef PLATFORM_THREADING  // 关闭debug 任务保护

#if PLATFORM_THREADING
#include "concurrent_hal.h"
#endif

LoggerOutputLevel log_level_at_run_time = LOG_LEVEL_AT_RUN_TIME;
debug_output_fn debug_output_;

#if PLATFORM_THREADING
static os_mutex_recursive_t debug_mutex = 0;

void init_debug_mutex(void)
{
    os_mutex_recursive_create(&debug_mutex);
}

static void __debug_lock(void)
{
    if (debug_mutex)
        os_mutex_recursive_lock(debug_mutex);
}

static void __debug_unlock(void)
{
    if (debug_mutex)
        os_mutex_recursive_unlock(debug_mutex);
}

#define LOCK()      __debug_lock()
#define UNLOCK()    __debug_unlock()

#else

void init_debug_mutex(void)
{
}

#define LOCK()
#define UNLOCK()

#endif

void set_logger_output(debug_output_fn output, LoggerOutputLevel level)
{
    if (output)
        debug_output_ = output;
    if (level==DEFAULT_LEVEL)
        level = LOG_LEVEL_AT_RUN_TIME;
    log_level_at_run_time = level;
}

void log_print_(int level, int line, const char *func, const char *file, const char *msg, ...)
{
    LOCK();

    if (level<log_level_at_run_time || !debug_output_)
        return;

    char _buffer[MAX_DEBUG_MESSAGE_LENGTH];
    static const char * levels[] = {
        "",
        "LOG  ",
        "DEBUG",
        "INFO ",
        "WARN ",
        "ERROR",
        "PANIC",
    };
    va_list args;
    va_start(args, msg);
    file = file ? strrchr(file,'/') + 1 : "";
    int trunc = snprintf(_buffer, arraySize(_buffer), "[%010u]:[%s]:[%s][%s](%d):", (unsigned)HAL_Timer_Get_Milli_Seconds(), levels[level/10], func, file, line);
    debug_output_(_buffer);
    if (trunc > arraySize(_buffer))
    {
        debug_output_("...");
    }
    trunc = vsnprintf(_buffer, arraySize(_buffer), msg, args);
    debug_output_(_buffer);
    if (trunc > arraySize(_buffer))
    {
        debug_output_("...");
    }
    debug_output_("\r\n");
    va_end(args);

    UNLOCK();
}

void log_print_simple_(int level, int line, const char *file, const char *msg, ...)
{
    LOCK();

    if (level<log_level_at_run_time || !debug_output_)
        return;

    char _buffer[MAX_DEBUG_MESSAGE_LENGTH];
    va_list args;
    va_start(args, msg);
    file = file ? strrchr(file,'/') + 1 : "";
    int trunc = snprintf(_buffer, arraySize(_buffer), "[%010u]:[%25s](%04d):->  ", (unsigned)HAL_Timer_Get_Milli_Seconds(), file, line);
    debug_output_(_buffer);
    if (trunc > arraySize(_buffer))
    {
        debug_output_("...");
    }
    trunc = vsnprintf(_buffer, arraySize(_buffer), msg, args);
    debug_output_(_buffer);
    if (trunc > arraySize(_buffer))
    {
        debug_output_("...");
    }
    debug_output_("\r\n");
    va_end(args);

    UNLOCK();
}

void log_print_direct_(int level, void* reserved, const char *msg, ...)
{
    LOCK();

    if (level<log_level_at_run_time || !debug_output_)
        return;

    char _buffer[MAX_DEBUG_MESSAGE_LENGTH];
    va_list args;
    va_start(args, msg);
    int trunc = vsnprintf(_buffer, arraySize(_buffer), msg, args);
    debug_output_(_buffer);
    if (trunc > arraySize(_buffer))
    {
        debug_output_("...");
    }
    va_end(args);

    UNLOCK();

}

void log_direct_(const char* s) {
    LOCK();

    if (LOG_LEVEL<log_level_at_run_time || !debug_output_)
        return;

    debug_output_(s);

    UNLOCK();
}

int log_level_active(LoggerOutputLevel level, void* reserved)
{
    return (level>=log_level_at_run_time && debug_output_);
}

