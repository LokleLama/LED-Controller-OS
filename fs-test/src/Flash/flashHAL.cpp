#include "Flash/flashHAL.h"

uint8_t* FlashHAL::flash_memory_pointer = nullptr;

void FlashHAL::flash_range_erase(uint32_t flash_offs, size_t count){
    if(calculateSectorAddress(calculateSector((int)flash_offs)) != (int)flash_offs) {
      printf("ERROR: Flash erase start address is not aligned to sector start: %u\n", (int)flash_offs);
      return; // not aligned to sector start
    }
    if(calculateSectorAddress(calculateSector((int)count)) != (int)count) {
      printf("ERROR: Flash erase size is not aligned to sector size: %u\n", (int)count);
      return; // not aligned to sector size
    }
    std::memset((uint8_t*)getFlashMemoryOffset() + flash_offs, 0xFF, count);
  }

void FlashHAL::flash_range_program(uint32_t flash_offs, const uint8_t *data,
                                  size_t count){
    if(calculatePageAddress(calculatePage((int)flash_offs)) != (int)flash_offs) {
      printf("ERROR: Flash program start address is not aligned to page start: %u\n", (int)flash_offs);
      return; // not aligned to page start
    }
    if(calculatePageAddress(calculatePage((int)count)) != (int)count) {
      printf("ERROR: Flash program size is not aligned to page size: %u\n", (int)count);
      return; // not aligned to page size
    }

    uint8_t* flash_ptr = (uint8_t*)getFlashMemoryOffset() + flash_offs;
    for(size_t i = 0; i < count; i++) {
      flash_ptr[i] &= data[i];
    }
  }