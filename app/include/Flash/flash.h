#pragma once

#include <map>
#include <string>
#include <vector>
#include <cstdint>

class Flash {
public:
  static const void *getAddress(size_t offset);
  static size_t getOffset(const void* address);
  
  static int read(void* buffer, size_t size, const void* address);
  static int read(std::vector<uint8_t> &buffer, const void* address);
  static int read(std::vector<uint8_t> &buffer, size_t offset);
  static int read(std::vector<uint16_t> &buffer, const void* address);
  static int read(std::vector<uint16_t> &buffer, size_t offset);
  static int read(std::vector<uint32_t> &buffer, const void* address);
  static int read(std::vector<uint32_t> &buffer, size_t offset);

  static int write(const void* buffer, size_t size, const void* address);
  static int write(const std::vector<uint8_t> &buffer, const void* address);
  static int write(const std::vector<uint8_t> &buffer, size_t offset);
  static int write(const std::vector<uint16_t> &buffer, const void* address);
  static int write(const std::vector<uint16_t> &buffer, size_t offset);
  static int write(const std::vector<uint32_t> &buffer, const void* address);
  static int write(const std::vector<uint32_t> &buffer, size_t offset);

  static int erase(const void* address, int length);

  static constexpr size_t MAX_FLASH_SIZE = 16 * 1024 * 1024;
};
