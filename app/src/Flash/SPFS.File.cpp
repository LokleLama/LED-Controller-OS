#include "SPFS.h"
#include "flash.h"
#include <cstring>

const std::string SPFS::File::getName() const {
  const char* name_ptr = reinterpret_cast<const char*>(_header) + sizeof(SPFS::FileHeader);
  return std::string(name_ptr, _header->name_size_meta_offset & 0x00FF);
}

