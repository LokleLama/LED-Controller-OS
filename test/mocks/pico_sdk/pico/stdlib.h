// Stub: pico/stdlib.h — minimal types for host-side testing
#pragma once

#include <cstdint>
#include <cstddef>

#ifndef uint
typedef unsigned int uint;
#endif

typedef uint64_t absolute_time_t;

static inline void stdio_init_all(void) {}
static inline void sleep_ms(uint32_t) {}
static inline void sleep_us(uint64_t) {}
static inline void tight_loop_contents(void) {}
static inline absolute_time_t get_absolute_time(void) { return 0; }
static inline uint32_t to_ms_since_boot(absolute_time_t t) { return (uint32_t)(t / 1000); }

typedef bool (*repeating_timer_callback_t)(struct repeating_timer *);
typedef struct repeating_timer {
    int64_t delay_us;
    repeating_timer_callback_t callback;
    void *user_data;
} repeating_timer_t;

static inline bool add_repeating_timer_ms(int32_t, repeating_timer_callback_t, void *, repeating_timer_t *t) {
    return true;
}
static inline bool cancel_repeating_timer(repeating_timer_t *) { return true; }
