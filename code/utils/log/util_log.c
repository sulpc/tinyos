#include "util_log.h"
#include "util_misc.h"
#include "util_ringbuffer.h"

#include <stdarg.h>   // va_list
#include <stdio.h>    // vsprintf

#define DEFAULT_SYS_LOG_LEVEL  LOG_ERROR
#define DEFAULT_SYS_LOG_SWITCH LOG_OFF

typedef int (*log_tx_func_t)(const uint8_t* msg, uint16_t msg_len);

typedef struct {
    log_level_t       log_level;
    bool              log_enable;
    log_tx_func_t     tx_func;
    util_ringbuffer_t buffer;
} channel_cfg_t;


static bool          log_buffer_inited  = false;
static const char*   log_level_prefix[] = {LOG_LEVEL_PREFIXS};
static uint8_t       log_data_buffer[LOG_BUFFER_SIZE];
static channel_cfg_t log_ch_cfg = {
    .log_level  = DEFAULT_SYS_LOG_LEVEL,
    .log_enable = DEFAULT_SYS_LOG_SWITCH,
    .tx_func    = console_put,
};


void util_printf(const char* fmt, ...)
{
    static char buffer[LOG_LINE_BUFFER_SIZE];
    va_list     ap;

    // ! todo: lock
    if (!log_buffer_inited) {
        util_ringbuffer_init(&log_ch_cfg.buffer, &log_data_buffer[0], LOG_BUFFER_SIZE);
        log_buffer_inited = true;
    }

    va_start(ap, fmt);
    vsprintf(buffer, fmt, ap);
    va_end(ap);

    util_ringbuffer_put(&log_ch_cfg.buffer, (uint8_t*)buffer, (uint16_t)strlen(buffer));
    // ! todo: unlock
}


void util_printfx(bool log_sw, log_level_t log_level, const char* fmt, ...)
{
    static char buffer[LOG_LINE_BUFFER_SIZE];
    va_list     ap;

    if (log_level >= LOG_LEVEL_NUM) {
        return;
    }

    if (log_sw == LOG_OFF || log_ch_cfg.log_enable == LOG_OFF || log_level < log_ch_cfg.log_level) {
        return;
    }

    // ! todo: lock
    if (!log_buffer_inited) {
        util_ringbuffer_init(&log_ch_cfg.buffer, &log_data_buffer[0], LOG_BUFFER_SIZE);
        log_buffer_inited = true;
    }

    // add prefix, like: time, level...
    int n = sprintf(buffer, log_level_prefix[log_level]);

    va_start(ap, fmt);
    vsprintf(buffer + n, fmt, ap);
    va_end(ap);

    // add into buffer
    util_ringbuffer_put(&log_ch_cfg.buffer, (uint8_t*)buffer, (uint16_t)strlen(buffer));

    // ! todo: unlock
}


void util_log_process(void)
{
    static char buffer[LOG_LINE_BUFFER_SIZE];

    if (log_buffer_inited) {
        // ! todo: lock
        uint16_t data_size = util_ringbuffer_get(&log_ch_cfg.buffer, (uint8_t*)buffer, sizeof(buffer));
        log_ch_cfg.tx_func((uint8_t*)buffer, data_size);
        // ! todo: unlock
    }
}


void util_printk(const char* fmt, ...)
{
    static char buffer[LOG_LINE_BUFFER_SIZE];
    va_list     ap;

    va_start(ap, fmt);
    vsprintf(buffer, fmt, ap);
    va_end(ap);

    console_puts(buffer);
}


void util_printkx(bool log_sw, log_level_t log_level, const char* fmt, ...)
{
    static char buffer[LOG_LINE_BUFFER_SIZE];
    va_list     ap;

    if (log_sw != LOG_ON || log_level >= LOG_LEVEL_NUM)
        return;
    console_puts(log_level_prefix[log_level]);

    va_start(ap, fmt);
    vsprintf(buffer, fmt, ap);
    va_end(ap);

    console_puts(buffer);
}


void util_log_set_sys_level(log_level_t log_level)
{
    if (log_level >= LOG_LEVEL_NUM) {
        return;
    }
    log_ch_cfg.log_level = log_level;
}


void util_log_set_sys_enable(bool on_off)
{
    log_ch_cfg.log_enable = (on_off == LOG_ON) ? LOG_ON : LOG_OFF;
}
