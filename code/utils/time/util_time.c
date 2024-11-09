#include "util_time.h"

static inline bool is_leap_year(uint16_t year)
{
    if (year % 4 == 0) {
        if (year % 100 == 0) {
            if (year % 400 == 0)
                return true;
            else
                return false;
        } else
            return true;
    } else
        return false;
}

void util_localtime(uint32_t* t, util_time_t* tmp)
{
    const uint32_t secs_min  = 60;
    const uint32_t secs_hour = 3600;
    const uint32_t secs_day  = 3600 * 24;

    uint32_t days    = *t / secs_day; /* Days passed since epoch. */
    uint32_t seconds = *t % secs_day; /* Remaining seconds. */

    tmp->tm_hour = seconds / secs_hour;
    tmp->tm_min  = (seconds % secs_hour) / secs_min;
    tmp->tm_sec  = (seconds % secs_hour) % secs_min;

    /* Calculate the current year. */
    tmp->tm_year = 1970;

    while (1) {
        /* Leap years have one day more. */
        uint32_t days_this_year = 365 + is_leap_year(tmp->tm_year);
        if (days_this_year > days)
            break;
        days -= days_this_year;
        tmp->tm_year++;
    }

    /* We need to calculate in which month and day of the month we are. To do * so we need to skip days according to how
     * many days there are in each * month, and adjust for the leap year that has one more day in February. */
    uint8_t mdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    mdays[1] += is_leap_year(tmp->tm_year);

    tmp->tm_mon = 0;
    while (days >= mdays[tmp->tm_mon]) {
        days -= mdays[tmp->tm_mon];
        tmp->tm_mon++;
    }

    tmp->tm_mday = days + 1; /* Add 1 since our 'days' is zero-based. */
    tmp->tm_year -= 1900;    /* Surprisingly tm_year is year-1900. */
}

// return unix timestamp (seconds from 1970)
uint32_t util_mktime(util_time_t* tm)
{
    uint32_t seccount = 0;

    if (tm->tm_year < 1970 || tm->tm_year > 2099)
        return 0;

    // second of the years
    for (uint16_t y = 1970; y < tm->tm_year; y++) {
        if (is_leap_year(y))
            seccount += 31622400;   // leap year
        else
            seccount += 31536000;
    }

    // second of the months
    const uint8_t mdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (is_leap_year(tm->tm_year) && tm->tm_mon > 2) {
        seccount += 86400;   // leap year, add one day
    }
    while (tm->tm_mon > 1) {
        tm->tm_mon--;
        seccount += (uint32_t)mdays[tm->tm_mon] * 86400;
    }

    seccount += (uint32_t)(tm->tm_mday - 1) * 86400;   // second of the days
    seccount += (uint32_t)tm->tm_hour * 3600;          // second of the hours
    seccount += (uint32_t)tm->tm_min * 60;             // second of the minutes
    seccount += tm->tm_sec;                            //

    return seccount;
}
