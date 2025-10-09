#include "Flash/flash.h"
#include "Flash/flashHAL.h"
#include <cstring>
#include <stdexcept>


void *Flash::readPointer(int offset) {
  if (offset < 0 || offset >= (int)MAX_FLASH_SIZE) {
    return nullptr;
  }
  return (void *)((uint8_t *)FlashHAL::getFlashMemoryOffset() + offset);
}

int Flash::read(std::vector<uint8_t> &buffer, int offset) {
  if (offset < 0 || buffer.size() == 0 ||
      (offset + buffer.size() > (MAX_FLASH_SIZE))) {
    return 0; // Invalid parameters
  }
  memcpy(buffer.data(), (uint8_t *)FlashHAL::getFlashMemoryOffset() + offset, buffer.size());
  return buffer.size();
}

int Flash::read(std::vector<uint8_t> &buffer, const void* address) {
  if (address == nullptr || buffer.size() == 0 ||
      ((((uint8_t*)address) - ((uint8_t*)FlashHAL::getFlashMemoryOffset())) + buffer.size()) > (MAX_FLASH_SIZE)) {
    return 0; // Invalid parameters
  }
  memcpy(buffer.data(), address, buffer.size());
  return buffer.size();
}

int Flash::write(const std::vector<uint8_t> &buffer, const void* address) {
  int offset = (((uint8_t*)address) - ((uint8_t*)FlashHAL::getFlashMemoryOffset()));
  if (address == nullptr || buffer.size() == 0 ||
      offset + buffer.size() > (MAX_FLASH_SIZE)) {
    return -1; // Invalid parameters
  }

  int start_page = FlashHAL::calculatePage(offset);
  if (FlashHAL::calculatePageAddress(start_page) != offset) {
    return -2; // Address not aligned to page start
  }

  int page_count = FlashHAL::calculatePage(buffer.size());
  if (FlashHAL::calculatePageAddress(page_count) != (int)buffer.size()) {
    return -3; // Buffer size not aligned to page size
  }

  FlashHAL::flash_range_program(offset, buffer.data(), buffer.size());
  return buffer.size();
}

int Flash::erase(const void* address, int length) {
  int offset = (((uint8_t*)address) - ((uint8_t*)FlashHAL::getFlashMemoryOffset()));
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
