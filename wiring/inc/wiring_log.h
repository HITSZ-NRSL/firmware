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

#ifndef WIRING_LOG_H_
#define WIRING_LOG_H_

#include <stdint.h>
#include "molmc_log.h"

class LogClass
{
    public:
        LogClass();
        ~LogClass();

        void setLevel(const char* tag, molmc_log_level_t level);
        log_output_fn_t setOutput(log_output_fn_t func);
};

extern LogClass Log;

#endif
