/**
 * @file util_time.h
 * @author sulpc
 * @brief
 * @version 0.1
 * @date 2024-05-30
 *
 * @copyright Copyright (c) 2024
 *
 */
#ifndef _UTIL_TIME_H_
#define _UTIL_TIME_H_

#include "util_types.h"


typedef struct {
    uint16_t tm_year;   // years since 1900
    uint8_t  tm_mon;    // months since January - [0, 11]
    uint8_t  tm_mday;   // day of the month - [1, 31]
    uint8_t  tm_hour;   // hours since midnight - [0, 23]
    uint8_t  tm_min;    // minutes after the hour - [0, 59]
    uint8_t  tm_sec;    // seconds after the minute - [0, 60] including leap second
} util_time_t;


void     util_localtime(uint32_t* t, util_time_t* tm);
uint32_t util_mktime(util_time_t* tm);

#endif
