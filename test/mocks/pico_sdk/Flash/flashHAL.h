// Mock: Flash/flashHAL.h — heap-backed flash for host-side testing
// This replaces app/include/Flash/flashHAL.h when the mock include path
// is placed before app/include in the compiler search path.
#pragma once

#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cstdio>

class FlashHAL {
public:
  static void flash_range_erase(uint32_t flash_offs, size_t count);

  static void flash_range_program(uint32_t flash_offs, const uint8_t *data,
                                  size_t count);

  static void flash_get_unique_id(uint8_t *id_out) {
    std::memset(id_out, 0, 8);
  }

  static int calculateSector(int address) {
    return address / 4096;
  }
  static int calculateSectorAddress(int sector) {
    return sector * 4096;
  }
  static int calculatePage(int address) { return address / 256; }
  static int calculatePageAddress(int page) { return page * 256; }

  static void* getFlashMemoryOffset() { return flash_memory_pointer; }
  static void setFlashMemoryOffset(void* ptr) { flash_memory_pointer = (uint8_t*)ptr; }

private:
  static uint8_t* flash_memory_pointer;
};
