// Stub: hardware/timer.h — timer functions for host-side testing
#pragma once

#include <cstdint>
#include <cstddef>

typedef struct repeating_timer {
    int64_t delay_us;
    void *user_data;
    bool (*callback)(struct repeating_timer *);
} repeating_timer_t;

static inline uint64_t time_us_64(void) {
    static uint64_t t = 0;
    return t += 1000;
}

static inline bool add_repeating_timer_ms(int32_t, bool (*callback)(repeating_timer_t *), void *user_data, repeating_timer_t *out) {
    if (out) {
        out->callback = callback;
        out->user_data = user_data;
    }
    return true;
}

static inline bool cancel_repeating_timer(repeating_timer_t *) { return true; }
