#include "SPFS.h"

std::shared_ptr<SPFS::Directory>
SPFS::getRootDirectory(int start_address, int end_address, int size) {
  if (!findFileSystemStart(start_address, end_address)) {
    return nullptr;
  }

  return std::make_shared<Directory>(*this, nullptr, "root");
}

bool SPFS::initializeFileSystem(int size) { return true; }

bool SPFS::findFileSystemStart(int start_address, int end_address) {
  if (end_address == -1) {
    end_address = Flash::MAX_FLASH_SIZE;
  }

  int start_sector = (start_address + FS_ALIGNMENT - 1) / FS_ALIGNMENT;
  int end_sector = (end_address + FS_ALIGNMENT - 1) / FS_ALIGNMENT;

  for (int n = start_sector; n < end_sector; n++) {
    uint32_t *sector =
        static_cast<uint32_t *>(Flash::readPointer(n * FS_ALIGNMENT));
    if (sector == nullptr) {
      return false;
    }
    if (*sector == MAGIC_NUMBER) {
      _address = Flash::readPointer(n * FS_ALIGNMENT);
      return true;
    }
  }

  return false;
}
