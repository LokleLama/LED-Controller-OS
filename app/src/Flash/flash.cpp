#include "Flash/flash.h"
#include "Flash/flashHAL.h"
#include <cstring>
#include <stdexcept>


const void *Flash::getAddress(size_t offset) {
  if (offset >= MAX_FLASH_SIZE) {
    return nullptr;
  }
  return (const void *)((const uint8_t *)FlashHAL::getFlashMemoryOffset() + offset);
}

size_t Flash::getOffset(const void* address) {
  if (address == nullptr) {
    return 0;
  }
  if (address < FlashHAL::getFlashMemoryOffset()) {
    return 0;
  }
  auto offset = (size_t)((uint8_t*)address - (uint8_t*)FlashHAL::getFlashMemoryOffset());
  if(offset >= MAX_FLASH_SIZE) {
    return 0;
  }
  return offset;
}

int Flash::read(std::vector<uint8_t> &buffer, size_t offset) {
  return read(buffer, getAddress(offset));
}

int Flash::read(std::vector<uint8_t> &buffer, const void* address) {
  return read(buffer.data(), buffer.size(), address);
}

int Flash::read(std::vector<uint16_t> &buffer, size_t offset) {
  return read(buffer, getAddress(offset));
}

int Flash::read(std::vector<uint16_t> &buffer, const void* address) {
  return read(buffer.data(), buffer.size() * sizeof(uint16_t), address) / sizeof(uint16_t);
}

int Flash::read(std::vector<uint32_t> &buffer, size_t offset) {
  return read(buffer, getAddress(offset));
}

int Flash::read(std::vector<uint32_t> &buffer, const void* address) {
  return read(buffer.data(), buffer.size() * sizeof(uint32_t), address) / sizeof(uint32_t);
}

int Flash::read(void* buffer, size_t size, const void* address) {
  auto offset = getOffset(address);
  if (address == nullptr || buffer == nullptr || size == 0 ||
      offset > (MAX_FLASH_SIZE - size)) {
    return 0; // Invalid parameters
  }
  memcpy(buffer, address, size);
  return size;
}

int Flash::write(const std::vector<uint8_t> &buffer, size_t offset) {
  return write(buffer, getAddress(offset));
}

int Flash::write(const std::vector<uint8_t> &buffer, const void* address) {
  return write(buffer.data(), buffer.size(), address);
}

int Flash::write(const std::vector<uint16_t> &buffer, size_t offset) {
  return write(buffer, getAddress(offset));
}

int Flash::write(const std::vector<uint16_t> &buffer, const void* address) {
  return write(buffer.data(), buffer.size() * sizeof(uint16_t), address) / sizeof(uint16_t);
}

int Flash::write(const std::vector<uint32_t> &buffer, size_t offset) {
  return write(buffer, getAddress(offset));
}

int Flash::write(const std::vector<uint32_t> &buffer, const void* address) {
  return write(buffer.data(), buffer.size() * sizeof(uint32_t), address) / sizeof(uint32_t);
}

int Flash::write(const void* buffer, size_t size, const void* address) {
  size_t offset = getOffset(address);
  if (address == nullptr || size == 0 ||
      offset + size > (MAX_FLASH_SIZE)) {
    return -1; // Invalid parameters
  }

  int start_page = FlashHAL::calculatePage(offset);
  if (FlashHAL::calculatePageAddress(start_page) != offset) {
    return -2; // Address not aligned to page start
  }

  int page_count = FlashHAL::calculatePage(size);
  if (FlashHAL::calculatePageAddress(page_count) != (int)size) {
    return -3; // Buffer size not aligned to page size
  }

  FlashHAL::flash_range_program(offset, (const uint8_t*)buffer, size);
  return size;
}

int Flash::erase(const void* address, int length) {
  size_t offset = getOffset(address);
  if (address == nullptr || length <= 0 || ((offset + length) > (int)(MAX_FLASH_SIZE))) {
    return -1; // Invalid parameters
  }
  int start_sector = FlashHAL::calculateSector(offset);
  int sector_count = FlashHAL::calculateSector(length);

  if (FlashHAL::calculateSectorAddress(start_sector) != offset) {
    return -2; // Address not aligned to sector start
  }

  if (FlashHAL::calculateSectorAddress(sector_count) != length) {
    return -3; // Erase length not aligned to sector size
  }

  FlashHAL::flash_range_erase(offset, length);
  return 0;
}
