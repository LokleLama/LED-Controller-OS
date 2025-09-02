#pragma once

#include <map>
#include <string>
#include <vector>
#include <cstdint>

class Flash {
public:
  static int read(std::vector<uint8_t> &buffer, int address);
  static void *readPointer(int address);
  static int write(const std::vector<uint8_t> &buffer, int address);
  static int erase(int address, int length);

  static constexpr size_t MAX_FLASH_SIZE = 16 * 1024 * 1024;
};
