#include "Flash/flash.h"
#include "hardware/flash.h"
#include "pico/stdlib.h"
#include <cstring>

int Flash::read(std::vector<uint8_t> &buffer, int offset) {
  if (offset < 0 || buffer.size() == 0 ||
      offset + buffer.size() > (8 * 1024 * 1024)) {
    return -1; // Invalid parameters
  }
  memcpy(buffer.data(), (const void *)(XIP_BASE + offset), buffer.size());
  return 0;
}

int Flash::write(const std::vector<uint8_t> &buffer, int offset) {
  if (offset < 0 || buffer.size() == 0 ||
      offset + buffer.size() > (8 * 1024 * 1024)) {
    return -1; // Invalid parameters
  }
  memcpy((void *)(XIP_BASE + offset), buffer.data(), buffer.size());
  return 0;
}

int Flash::erase(int offset, int length) {
  // Implementation for erasing flash memory
  return 0;
}
