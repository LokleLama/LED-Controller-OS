// Stub: hardware/rtc.h — RTC types for host-side testing
#pragma once

#include <cstdint>

typedef struct {
    int16_t year;
    int8_t  month;
    int8_t  day;
    int8_t  dotw;
    int8_t  hour;
    int8_t  min;
    int8_t  sec;
} datetime_t;

static inline void rtc_init(void) {}
static inline bool rtc_set_datetime(const datetime_t *) { return true; }
static inline bool rtc_get_datetime(datetime_t *t) {
    if (t) { *t = {}; }
    return true;
}
static inline bool rtc_running(void) { return true; }
