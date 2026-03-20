// Mock FlashHAL backend — heap-based flash simulation
// Provides FlashHAL static member implementations using a heap buffer.
// Tests must call mock_flash_init() before any flash operations and
// mock_flash_cleanup() when done.

#include "Flash/flashHAL.h"
#include <cstdlib>
#include <cstdio>

// Static member definition
uint8_t* FlashHAL::flash_memory_pointer = nullptr;

void FlashHAL::flash_range_erase(uint32_t flash_offs, size_t count) {
    if (!flash_memory_pointer) { return; }
    if (calculateSectorAddress(calculateSector((int)flash_offs)) != (int)flash_offs) {
        printf("ERROR: Flash erase start address is not aligned to sector start: %u\n", flash_offs);
        return;
    }
    if (calculateSectorAddress(calculateSector((int)count)) != (int)count) {
        printf("ERROR: Flash erase size is not aligned to sector size: %u\n", (unsigned)count);
        return;
    }
    std::memset(flash_memory_pointer + flash_offs, 0xFF, count);
}

void FlashHAL::flash_range_program(uint32_t flash_offs, const uint8_t *data,
                                   size_t count) {
    if (!flash_memory_pointer || !data) { return; }
    if (calculatePageAddress(calculatePage((int)flash_offs)) != (int)flash_offs) {
        printf("ERROR: Flash program start address is not aligned to page start: %u\n", flash_offs);
        return;
    }
    if (calculatePageAddress(calculatePage((int)count)) != (int)count) {
        printf("ERROR: Flash program size is not aligned to page size: %u\n", (unsigned)count);
        return;
    }
    // Flash programming: can only clear bits (AND operation)
    uint8_t *dst = flash_memory_pointer + flash_offs;
    for (size_t i = 0; i < count; i++) {
        dst[i] &= data[i];
    }
}

// Also provide C-level stubs (not used when FlashHAL mock intercepts, but
// needed if any code calls ::flash_range_erase directly)
extern "C" {
void flash_range_erase(uint32_t, size_t) {}
void flash_range_program(uint32_t, const uint8_t *, size_t) {}
void flash_get_unique_id(uint8_t *id_out) {
    if (id_out) std::memset(id_out, 0x42, 8);
}
}

// --- Forward declarations for test helpers ---
void mock_flash_cleanup();

// --- Helpers for test setup/teardown ---

uint8_t *mock_flash_init(size_t size) {
    mock_flash_cleanup();
    uint8_t *buf = static_cast<uint8_t *>(std::malloc(size));
    if (buf) {
        std::memset(buf, 0xFF, size);
        FlashHAL::setFlashMemoryOffset(buf);
    }
    return buf;
}

void mock_flash_cleanup() {
    void *buf = FlashHAL::getFlashMemoryOffset();
    if (buf) {
        std::free(buf);
        FlashHAL::setFlashMemoryOffset(nullptr);
    }
}

uint8_t *mock_flash_get_buffer() {
    return static_cast<uint8_t *>(FlashHAL::getFlashMemoryOffset());
}

size_t mock_flash_get_size() {
    // Not tracked here; tests should track their own size
    return 0;
}
