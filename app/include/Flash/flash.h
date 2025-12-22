#pragma once

#include <map>
#include <string>
#include <vector>
#include <cstdint>

class Flash {
public:
  static void *readPointer(size_t offset);
  static int read(std::vector<uint8_t> &buffer, const void* address);
  static int read(std::vector<uint8_t> &buffer, size_t offset);
  static int write(const std::vector<uint8_t> &buffer, const void* address);
  static int erase(const void* address, int length);

  static constexpr size_t MAX_FLASH_SIZE = 16 * 1024 * 1024;
};
