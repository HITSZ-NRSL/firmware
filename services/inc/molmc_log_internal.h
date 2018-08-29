/*
 * Copyright (c) 2013-2018 Molmc Group. All rights reserved.
 * License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef __MOLMC_LOG_INTERNAL_H__
#define __MOLMC_LOG_INTERNAL_H__

//these functions do not check level versus MOLMC_LOCAL_LEVEL, this should be done in iotx_log_api.h
void molmc_log_buffer_hex_internal(const char *tag, const void *buffer, uint16_t buff_len, molmc_log_level_t level);
void molmc_log_buffer_char_internal(const char *tag, const void *buffer, uint16_t buff_len, molmc_log_level_t level);
void molmc_log_buffer_hexdump_internal( const char *tag, const void *buffer, uint16_t buff_len, molmc_log_level_t log_level);

#endif
