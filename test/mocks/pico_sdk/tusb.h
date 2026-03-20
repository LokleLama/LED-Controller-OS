// Stub: tusb.h — TinyUSB CDC stubs for host-side testing
#pragma once

#include <cstdint>
#include <cstddef>

static inline void tusb_init(void) {}
static inline void tud_task(void) {}
static inline bool tud_cdc_n_connected(uint8_t) { return false; }
static inline uint32_t tud_cdc_n_available(uint8_t) { return 0; }
static inline uint32_t tud_cdc_n_read(uint8_t, void *, uint32_t) { return 0; }
static inline uint32_t tud_cdc_n_write(uint8_t, const void *, uint32_t count) { return count; }
static inline uint32_t tud_cdc_n_write_flush(uint8_t) { return 0; }
