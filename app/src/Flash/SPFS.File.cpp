#include "SPFS.h"
#include "flash.h"
#include <cstring>

const std::string SPFS::File::getName() const {
  const char* name_ptr = reinterpret_cast<const char*>(_header) + ((_header->name_offset_size & 0xFF00) >> 8);
  return std::string(name_ptr, _header->name_offset_size & 0x00FF);
}

