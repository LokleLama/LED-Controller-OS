// Public API for mock flash — include in test files
#pragma once

#include <cstdint>
#include <cstddef>

/// Allocate simulated flash of given size, filled with 0xFF.
/// Returns pointer to the buffer.
uint8_t *mock_flash_init(size_t size);

/// Free the simulated flash buffer.
void mock_flash_cleanup();

/// Get current flash buffer pointer.
uint8_t *mock_flash_get_buffer();

/// Get current flash buffer size.
size_t mock_flash_get_size();
