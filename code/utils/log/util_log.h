/**
 * @file util_log.h
 * @author sulpc
 * @brief
 * @version 0.1
 * @date 2024-05-30
 *
 * @copyright Copyright (c) 2024
 *
 */
#ifndef _UTIL_LOG_H_
#define _UTIL_LOG_H_

#include "util_types.h"

#define LOG_BUFFER_SIZE      1024
#define LOG_LINE_BUFFER_SIZE 128

#define LOG_ON  true
#define LOG_OFF false

typedef enum {
    LOG_INFO = 0,
    LOG_DEBUG,
    LOG_WARNING,
    LOG_ERROR,
    LOG_EXIT,
    LOG_LEVEL_NUM,
} log_level_t;

#define LOG_LEVEL_PREFIXS "INF: ", "DBG: ", "WRN: ", "ERR: ", "EXT: "

// printf -- print async, use buffer
void util_printf(const char* fmt, ...);
void util_printfx(bool log_sw, log_level_t log_level, const char* fmt, ...);

// printk -- print sync immdiately
void util_printk(const char* fmt, ...);
void util_printkx(bool log_sw, log_level_t log_level, const char* fmt, ...);

void util_log_process(void);
void util_log_set_sys_level(log_level_t log_level);
void util_log_set_sys_enable(bool on_off);

/**
 * @brief print string, should implements by user, CALLOUT
 *
 * @param str
 * @return int 0-ok
 */
extern int console_puts(const char* str);

/**
 * @brief print data, should implements by user, CALLOUT
 *
 * @param data
 * @param len
 * @return int 0-ok
 */
extern int console_put(const uint8_t* data, uint16_t len);

#endif
