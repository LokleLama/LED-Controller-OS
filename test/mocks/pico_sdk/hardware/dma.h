// Stub: hardware/dma.h — DMA types for host-side testing
#pragma once

#include <cstdint>
#include <cstddef>

#ifndef uint
typedef unsigned int uint;
#endif

typedef enum {
    DMA_SIZE_8  = 0,
    DMA_SIZE_16 = 1,
    DMA_SIZE_32 = 2
} dma_channel_transfer_size_t;

typedef struct {
    uint32_t ctrl;
} dma_channel_config;

static inline int dma_claim_unused_channel(bool) { return 0; }
static inline dma_channel_config dma_channel_get_default_config(uint) { return {}; }
static inline void channel_config_set_transfer_data_size(dma_channel_config *, dma_channel_transfer_size_t) {}
static inline void channel_config_set_read_increment(dma_channel_config *, bool) {}
static inline void channel_config_set_write_increment(dma_channel_config *, bool) {}
static inline void channel_config_set_dreq(dma_channel_config *, uint) {}
static inline void dma_channel_configure(uint, const dma_channel_config *, volatile void *, const volatile void *, uint, bool) {}
static inline void dma_channel_set_irq0_enabled(uint, bool) {}
static inline bool dma_channel_is_busy(uint) { return false; }
static inline void dma_channel_wait_for_finish_blocking(uint) {}

void mock_dma_transfer(uint channel, const volatile void *read_addr, uint32_t transfer_count);
static inline void dma_channel_transfer_from_buffer_now(uint channel, const volatile void *read_addr, uint32_t transfer_count) {
    mock_dma_transfer(channel, read_addr, transfer_count);
}
