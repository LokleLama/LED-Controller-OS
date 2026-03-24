// Mock PIO/DMA frame capture for host-side testing
#pragma once

#include <vector>
#include <cstdint>
#include <functional>

using FrameCallback = std::function<void(const std::vector<uint32_t>&)>;

// Get the last complete frame transferred via PIO (DMA or blocking put)
const std::vector<uint32_t>& mock_pio_get_last_frame();

// Set a callback that fires after each DMA frame transfer
void mock_pio_set_frame_callback(FrameCallback cb);

// Reset captured state
void mock_pio_reset();
