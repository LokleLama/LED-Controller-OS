#include "Flash/flash.h"
#include "Flash/flashHAL.h"
#include "pico/stdlib.h"
#include <cstring>
#include <stdexcept>

int Flash::read(std::vector<uint8_t> &buffer, int address) {
  if (address < 0 || buffer.size() == 0 ||
      address + buffer.size() > (MAX_FLASH_SIZE)) {
    return -1; // Invalid parameters
  }
  memcpy(buffer.data(), (const void *)(readPointer(address)), buffer.size());
  return 0;
}

static void *Flash::readPointer(int address) {
  if (address < 0 || address >= MAX_FLASH_SIZE) {
    return nullptr;
  }
  return (void *)(XIP_BASE + address);
}

int Flash::write(const std::vector<uint8_t> &buffer, int address) {
  if (address < 0 || buffer.size() == 0 ||
      address + buffer.size() > (MAX_FLASH_SIZE)) {
    return -1; // Invalid parameters
  }

  int start_page = FlashHAL::calculatePage(address);
  if (FlashHAL::calculatePageAddress(start_page) != address) {
    return -2; // Address not aligned to page start
  }

  int page_count = FlashHAL::calculatePage(buffer.size());
  if (FlashHAL::calculatePageAddress(page_count) != buffer.size()) {
    return -3; // Buffer size not aligned to page size
  }

  FlashHAL::flash_range_program(address, buffer.data(), buffer.size());
  return 0;
}

int Flash::erase(int address, int length) {
  if (address < 0 || length <= 0 || address + length > (MAX_FLASH_SIZE)) {
    return -1; // Invalid parameters
  }
  int start_sector = FlashHAL::calculateSector(address);
  int sector_count = FlashHAL::calculateSector(length);

  if (FlashHAL::calculateSectorAddress(start_sector) != address) {
    return -2; // Address not aligned to sector start
  }

  if (FlashHAL::calculateSectorAddress(sector_count) != length) {
    return -3; // Erase length not aligned to sector size
  }

  FlashHAL::flash_range_erase(FlashHAL::calculateSectorAddress(start_sector),
                              FLASH_SECTOR_SIZE);
  return 0;
}
