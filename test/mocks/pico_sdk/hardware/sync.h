// Stub: hardware/sync.h — synchronization primitives for host-side testing
#pragma once

#include <cstdint>

static inline uint32_t save_and_disable_interrupts(void) { return 0; }
static inline void restore_interrupts(uint32_t) {}
