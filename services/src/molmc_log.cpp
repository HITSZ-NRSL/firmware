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

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include "sys/queue.h"
#include "molmc_log.h"
#include "wiring_ticks.h"
#include "wiring_usartserial.h"

#if PLATFORM_THREADING
#include "concurrent_hal.h"
#endif

//print number of bytes per line for molmc_log_buffer_char and molmc_log_buffer_hex
#define BYTES_PER_LINE                  16

// Number of tags to be cached. Must be 2**n - 1, n >= 2.
#define TAG_CACHE_SIZE                  31

#define MAX_DEBUG_MESSAGE_LENGTH        256

// Uncomment this to enable consistency checks and cache statistics in this file.
// #define LOG_BUILTIN_CHECKS

typedef struct {
    const char* tag;
    uint32_t level : 3;
    uint32_t generation : 29;
} cached_tag_entry_t;

typedef struct uncached_tag_entry_{
    SLIST_ENTRY(uncached_tag_entry_) entries;
    uint8_t level;  // molmc_log_level_t as uint8_t
    char tag[0];    // beginning of a zero-terminated string
} uncached_tag_entry_t;

static void log_output(const char* msg);

static molmc_log_level_t s_log_default_level = MOLMC_LOG_NONE;
static SLIST_HEAD(log_tags_head , uncached_tag_entry_) s_log_tags = SLIST_HEAD_INITIALIZER(s_log_tags);
static cached_tag_entry_t s_log_cache[TAG_CACHE_SIZE];
static uint32_t s_log_cache_max_generation = 0;
static uint32_t s_log_cache_entry_count = 0;
static log_output_fn_t s_log_print_func = &log_output;

#ifdef LOG_BUILTIN_CHECKS
static uint32_t s_log_cache_misses = 0;
#endif

static inline bool get_cached_log_level(const char* tag, molmc_log_level_t* level);
static inline bool get_uncached_log_level(const char* tag, molmc_log_level_t* level);
static inline void add_to_cache(const char* tag, molmc_log_level_t level);
static void heap_bubble_down(int index);
static inline void heap_swap(int i, int j);
static inline bool should_output(molmc_log_level_t level_for_message, molmc_log_level_t level_for_tag);
static inline void clear_log_level_list();

#if PLATFORM_THREADING
static os_mutex_recursive_t s_log_mutex = 0;

static void init_debug_mutex(void)
{
    if (!s_log_mutex) {
        os_mutex_recursive_create(&s_log_mutex);
    }
}

static void __debug_lock(void)
{
    if (s_log_mutex) {
        os_mutex_recursive_lock(s_log_mutex);
    }
}

static void __debug_unlock(void)
{
    if (s_log_mutex) {
        os_mutex_recursive_unlock(s_log_mutex);
    }
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

static void log_output(const char* msg)
{
    if(!Serial.isEnabled()) {
        Serial.begin(115200);
    } else {
        Serial.print(msg);
    }
}

log_output_fn_t molmc_log_set_output(log_output_fn_t func)
{
    init_debug_mutex();
    LOCK();

    log_output_fn_t orig_func = s_log_print_func;
    s_log_print_func = func;

    UNLOCK();
    return orig_func;
}

void molmc_log_level_set(const char* tag, molmc_log_level_t level)
{
    init_debug_mutex();
    LOCK();

    // for wildcard tag, remove all linked list items and clear the cache
    if (strcmp(tag, "*") == 0) {
        s_log_default_level = level;
        clear_log_level_list();
        UNLOCK();
        return;
    }

    //searching exist tag
    uncached_tag_entry_t *it = NULL;
    SLIST_FOREACH( it, &s_log_tags, entries ) {
        if ( strcmp(it->tag, tag)==0 ) {
            //one tag in the linked list match, update the level
            it->level = level;
            //quit with it != NULL
            break;
        }
    }
    //no exist tag, append new one
    if ( it == NULL ) {
        // allocate new linked list entry and append it to the head of the list
        size_t entry_size = offsetof(uncached_tag_entry_t, tag) + strlen(tag) + 1;
        uncached_tag_entry_t* new_entry = (uncached_tag_entry_t*) malloc(entry_size);
        if (!new_entry) {
            UNLOCK();
            return;
        }
        new_entry->level = (uint8_t) level;
        strcpy(new_entry->tag, tag);
        SLIST_INSERT_HEAD( &s_log_tags, new_entry, entries );
    }

    //search in the cache and update it if exist
    for (int i = 0; i < s_log_cache_entry_count; ++i) {
#ifdef LOG_BUILTIN_CHECKS
        assert(i == 0 || s_log_cache[(i - 1) / 2].generation < s_log_cache[i].generation);
#endif
        if (strcmp(s_log_cache[i].tag,tag) == 0) {
            s_log_cache[i].level = level;
            break;
        }
    }
    UNLOCK();
}

static void clear_log_level_list()
{
    while( !SLIST_EMPTY(&s_log_tags)) {
        SLIST_REMOVE_HEAD(&s_log_tags, entries );
    }
    s_log_cache_entry_count = 0;
    s_log_cache_max_generation = 0;
#ifdef LOG_BUILTIN_CHECKS
    s_log_cache_misses = 0;
#endif
}

static inline bool get_cached_log_level(const char* tag, molmc_log_level_t* level)
{
    // Look for `tag` in cache
    int i;
    for (i = 0; i < s_log_cache_entry_count; ++i) {
#ifdef LOG_BUILTIN_CHECKS
        assert(i == 0 || s_log_cache[(i - 1) / 2].generation < s_log_cache[i].generation);
#endif
        if (s_log_cache[i].tag == tag) {
            break;
        }
    }
    if (i == s_log_cache_entry_count) { // Not found in cache
        return false;
    }
    // Return level from cache
    *level = (molmc_log_level_t) s_log_cache[i].level;
    // If cache has been filled, start taking ordering into account
    // (other options are: dynamically resize cache, add "dummy" entries
    //  to the cache; this option was chosen because code is much simpler,
    //  and the unfair behavior of cache will show it self at most once, when
    //  it has just been filled)
    if (s_log_cache_entry_count == TAG_CACHE_SIZE) {
        // Update item generation
        s_log_cache[i].generation = s_log_cache_max_generation++;
        // Restore heap ordering
        heap_bubble_down(i);
    }
    return true;
}

static inline void add_to_cache(const char* tag, molmc_log_level_t level)
{
    uint32_t generation = s_log_cache_max_generation++;
    // First consider the case when cache is not filled yet.
    // In this case, just add new entry at the end.
    // This happens to satisfy binary min-heap ordering.
    if (s_log_cache_entry_count < TAG_CACHE_SIZE) {
        s_log_cache[s_log_cache_entry_count] = (cached_tag_entry_t) {
            .tag = tag,
            .level = level,
            .generation = generation
        };
        ++s_log_cache_entry_count;
        return;
    }

    // Cache is full, so we replace the oldest entry (which is at index 0
    // because this is a min-heap) with the new one, and do bubble-down
    // operation to restore min-heap ordering.
    s_log_cache[0] = (cached_tag_entry_t) {
        .tag = tag,
        .level = level,
        .generation = generation
    };
    heap_bubble_down(0);
}

static inline bool get_uncached_log_level(const char* tag, molmc_log_level_t* level)
{
    // Walk the linked list of all tags and see if given tag is present in the list.
    // This is slow because tags are compared as strings.
    uncached_tag_entry_t *it;
    SLIST_FOREACH( it, &s_log_tags, entries ) {
        if (strcmp(tag, it->tag) == 0) {
            *level = it->level;
            return true;
        }
    }
    return false;
}

static inline bool should_output(molmc_log_level_t level_for_message, molmc_log_level_t level_for_tag)
{
    return level_for_message <= level_for_tag;
}

static void heap_bubble_down(int index)
{
    while (index < TAG_CACHE_SIZE / 2) {
        int left_index = index * 2 + 1;
        int right_index = left_index + 1;
        int next = (s_log_cache[left_index].generation < s_log_cache[right_index].generation) ? left_index : right_index;
        heap_swap(index, next);
        index = next;
    }
}

static inline void heap_swap(int i, int j)
{
    cached_tag_entry_t tmp = s_log_cache[i];
    s_log_cache[i] = s_log_cache[j];
    s_log_cache[j] = tmp;
}

static bool molmc_ptr_byte_accessible(const void *p)
{
    return true;
}

uint32_t molmc_log_timestamp()
{
    return millis();
}

bool molmc_log_should_output(const char* tag, molmc_log_level_t level)
{
    if ( MOLMC_LOG_LOCAL_LEVEL < (level) ) {
        return false;
    }

    init_debug_mutex();

    LOCK();

    molmc_log_level_t level_for_tag;
    // Look for the tag in cache first, then in the linked list of all tags
    if (!get_cached_log_level(tag, &level_for_tag)) {
        if (!get_uncached_log_level(tag, &level_for_tag)) {
            level_for_tag = s_log_default_level;
        }
        add_to_cache(tag, level_for_tag);
#ifdef LOG_BUILTIN_CHECKS
        ++s_log_cache_misses;
#endif
    }
    UNLOCK();
    if (!should_output(level, level_for_tag)) {
        return false;
    }
    return true;
}

void molmc_log_write(const char* tag, molmc_log_level_t level, const char* format, ...)
{
    char _buffer[MAX_DEBUG_MESSAGE_LENGTH];

    if(!molmc_log_should_output(tag, level)) {
        return;
    }

    va_list list;
    va_start(list, format);
    int trunc = vsnprintf(_buffer, sizeof(_buffer), format, list);
    s_log_print_func(_buffer);
    if (trunc > (int)sizeof(_buffer)) {
       s_log_print_func("...\r\n");
    }
    va_end(list);
}

void molmc_log_buffer_hex_internal(const char *tag, molmc_log_level_t level, const void *buffer, uint16_t buff_len)
{
    if ( buff_len == 0 ) return;
    char temp_buffer[BYTES_PER_LINE+3];   //for not-byte-accessible memory
    char hex_buffer[3*BYTES_PER_LINE+1];
    const char *ptr_line;
    const char *ptr_buffer = buffer;
    int bytes_cur_line;

    if(!molmc_log_should_output(tag, level)) {
        return;
    }

    do {
        if ( buff_len > BYTES_PER_LINE ) {
            bytes_cur_line = BYTES_PER_LINE;
        } else {
            bytes_cur_line = buff_len;
        }
        if ( !molmc_ptr_byte_accessible(ptr_buffer) ) {
            //use memcpy to get around alignment issue
            memcpy( temp_buffer, ptr_buffer, (bytes_cur_line+3)/4*4 );
            ptr_line = temp_buffer;
        } else {
            ptr_line = ptr_buffer;
        }

        for( int i = 0; i < bytes_cur_line; i ++ ) {
            sprintf( hex_buffer + 3*i, "%02x ", (uint8_t)ptr_line[i] );
        }
        molmc_log_write( tag, level, "%s\r\n", hex_buffer );
        ptr_buffer += bytes_cur_line;
        buff_len -= bytes_cur_line;
    } while( buff_len );
}

void molmc_log_buffer_char_internal(const char *tag, molmc_log_level_t level, const void *buffer, uint16_t buff_len)
{
    if ( buff_len == 0 ) return;
    char temp_buffer[BYTES_PER_LINE+3];   //for not-byte-accessible memory
    char char_buffer[BYTES_PER_LINE+1];
    const char *ptr_line;
    const char *ptr_buffer = buffer;
    int bytes_cur_line;

    if(!molmc_log_should_output(tag, level)) {
        return;
    }

    do {
        if ( buff_len > BYTES_PER_LINE ) {
            bytes_cur_line = BYTES_PER_LINE;
        } else {
            bytes_cur_line = buff_len;
        }
        if ( !molmc_ptr_byte_accessible(ptr_buffer) ) {
            //use memcpy to get around alignment issue
            memcpy( temp_buffer, ptr_buffer, (bytes_cur_line+3)/4*4 );
            ptr_line = temp_buffer;
        } else {
            ptr_line = ptr_buffer;
        }

        for( int i = 0; i < bytes_cur_line; i ++ ) {
            sprintf( char_buffer + i, "%c", ptr_line[i] );
        }
        molmc_log_write( tag, level, "%s\r\n", char_buffer );
        ptr_buffer += bytes_cur_line;
        buff_len -= bytes_cur_line;
    } while( buff_len );
}

void molmc_log_buffer_hexdump_internal(const char *tag, molmc_log_level_t level, const void *buffer, uint16_t buff_len)
{
    if ( buff_len == 0 ) return;
    char temp_buffer[BYTES_PER_LINE+3];   //for not-byte-accessible memory
    const char *ptr_line;
    const char *ptr_buffer = buffer;
    //format: field[length]
    // ADDR[10]+"   "+DATA_HEX[8*3]+" "+DATA_HEX[8*3]+"  |"+DATA_CHAR[8]+"|"
    char hd_buffer[10+3+BYTES_PER_LINE*3+3+BYTES_PER_LINE+1+1];
    char *ptr_hd;
    int bytes_cur_line;

    if(!molmc_log_should_output(tag, level)) {
        return;
    }

    do {
        if ( buff_len > BYTES_PER_LINE ) {
            bytes_cur_line = BYTES_PER_LINE;
        } else {
            bytes_cur_line = buff_len;
        }
        if ( !molmc_ptr_byte_accessible(ptr_buffer) ) {
            //use memcpy to get around alignment issue
            memcpy( temp_buffer, ptr_buffer, (bytes_cur_line+3)/4*4 );
            ptr_line = temp_buffer;
        } else {
            ptr_line = ptr_buffer;
        }
        ptr_hd = hd_buffer;

        ptr_hd += sprintf( ptr_hd, "%p ", ptr_buffer );
        for( int i = 0; i < BYTES_PER_LINE; i ++ ) {
            if ( (i&7)==0 ) {
                ptr_hd += sprintf( ptr_hd, " " );
            }
            if ( i < bytes_cur_line ) {
                ptr_hd += sprintf( ptr_hd, " %02x", (uint8_t)ptr_line[i] );
            } else {
                ptr_hd += sprintf( ptr_hd, "   " );
            }
        }
        ptr_hd += sprintf( ptr_hd, "  |" );
        for( int i = 0; i < bytes_cur_line; i ++ ) {
            if ( isprint((int)ptr_line[i]) ) {
                ptr_hd += sprintf( ptr_hd, "%c", ptr_line[i] );
            } else {
                ptr_hd += sprintf( ptr_hd, "." );
            }
        }
        ptr_hd += sprintf( ptr_hd, "|" );

        molmc_log_write( tag, level, "%s\r\n", hd_buffer );
        ptr_buffer += bytes_cur_line;
        buff_len -= bytes_cur_line;
    } while( buff_len );
}
