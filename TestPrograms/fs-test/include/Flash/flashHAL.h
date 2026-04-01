#pragma once

#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cstdio>


class FlashHAL {
public:
  /*! \brief  Erase areas of flash
   *  \ingroup hardware_flash
   *
   * \param flash_offs Offset into flash, in bytes, to start the erase. Must be
   * aligned to a 4096-byte flash sector.
   * \param count Number of bytes to be erased. Must be a multiple of 4096 bytes
   * (one sector).
   */
  static void flash_range_erase(uint32_t flash_offs, size_t count);

  /*! \brief  Program flash
   *  \ingroup hardware_flash
   *
   * \param flash_offs Flash address of the first byte to be programmed. Must be
   * aligned to a 256-byte flash page.
   * \param data Pointer to the data to program into flash
   * \param count Number of bytes to program. Must be a multiple of 256 bytes
   * (one page).
   */
  static void flash_range_program(uint32_t flash_offs, const uint8_t *data,
                                  size_t count);

  /*! \brief Get flash unique 64 bit identifier
   *  \ingroup hardware_flash
   *
   * Use a standard 4Bh RUID instruction to retrieve the 64 bit unique
   * identifier from a flash device attached to the QSPI interface. Since there
   * is a 1:1 association between the MCU and this flash, this also serves as a
   * unique identifier for the board.
   *
   *  \param id_out Pointer to an 8-byte buffer to which the ID will be written
   */
  static void flash_get_unique_id(uint8_t *id_out) {
    std::memset(id_out, 0, 8);
  }

  static int calculateSector(int address) {
    return address / /*FLASH_SECTOR_SIZE*/4096;
  }
  static int calculateSectorAddress(int sector) {
    return sector * /*FLASH_SECTOR_SIZE*/4096;
  }
  static int calculatePage(int address) { return address / /*FLASH_PAGE_SIZE*/256; }
  static int calculatePageAddress(int page) { return page * /*FLASH_PAGE_SIZE*/256; }

  static void* getFlashMemoryOffset(){ return flash_memory_pointer; }

  static void setFlashMemoryOffset(void* ptr) { flash_memory_pointer = (uint8_t*)ptr; }

private:
  static uint8_t* flash_memory_pointer;
};