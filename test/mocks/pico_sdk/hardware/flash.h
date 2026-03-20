// Stub: hardware/flash.h — flash operations for host-side testing
#pragma once

#include <cstdint>
#include <cstddef>

#define FLASH_SECTOR_SIZE 4096u
#define FLASH_PAGE_SIZE   256u
#define XIP_BASE          0x10000000u

#ifdef __cplusplus
extern "C" {
#endif

void flash_range_erase(uint32_t flash_offs, size_t count);
void flash_range_program(uint32_t flash_offs, const uint8_t *data, size_t count);
void flash_get_unique_id(uint8_t *id_out);

#ifdef __cplusplus
}
#endif
