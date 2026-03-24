// Stub: hardware/gpio.h — GPIO functions for host-side testing
#pragma once

#include <cstdint>

static inline void gpio_init(uint) {}
static inline void gpio_set_dir(uint, bool) {}
static inline void gpio_set_function(uint, uint) {}
static inline void gpio_pull_up(uint) {}
static inline void gpio_pull_down(uint) {}
static inline bool gpio_get(uint) { return false; }
static inline void gpio_put(uint, bool) {}

#define GPIO_FUNC_UART 2
#define GPIO_FUNC_PIO0 6
#define GPIO_FUNC_PIO1 7
#define GPIO_OUT true
#define GPIO_IN  false
