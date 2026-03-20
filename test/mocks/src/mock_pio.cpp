// Mock PIO/DMA implementation — captures LED frame data for testing
#include "mock_pio.h"
#include "hardware/pio.h"

#include <cstring>
#include <vector>
#include <cstdint>

// PIO hardware instances (referenced by pio0/pio1 macros)
struct pio_hw pio0_hw_instance;
struct pio_hw pio1_hw_instance;

// Frame capture state
static std::vector<uint32_t> s_last_frame;
static FrameCallback s_frame_callback;

// Called by pio_sm_put_blocking stub (blocking transfer path, ≤16 LEDs)
void mock_pio_put(PIO, uint, uint32_t data) {
    s_last_frame.push_back(data);
}

// Called by dma_channel_transfer_from_buffer_now stub (DMA path, >16 LEDs)
void mock_dma_transfer(uint, const volatile void *read_addr, uint32_t transfer_count) {
    s_last_frame.clear();
    const uint32_t *ptr = reinterpret_cast<const uint32_t *>(const_cast<const void *>(read_addr));
    s_last_frame.assign(ptr, ptr + transfer_count);
    if (s_frame_callback) {
        s_frame_callback(s_last_frame);
    }
}

const std::vector<uint32_t>& mock_pio_get_last_frame() {
    return s_last_frame;
}

void mock_pio_set_frame_callback(FrameCallback cb) {
    s_frame_callback = cb;
}

void mock_pio_reset() {
    s_last_frame.clear();
    s_frame_callback = nullptr;
}
