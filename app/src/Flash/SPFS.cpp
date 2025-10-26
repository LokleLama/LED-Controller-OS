#include "SPFS.h"
#include "flash.h"
#include <cstring>
#include <cstring>

std::shared_ptr<SPFS::Directory> SPFS::getRootDirectory(int start_address, int end_address) {
  if(_fs_header == nullptr) {
    return findFileSystemStart(start_address, end_address);
  }

  return nullptr;
}

std::shared_ptr<SPFS::Directory> SPFS::findFileSystemStart(int start_address, int end_address) {
  if (end_address == -1) {
    end_address = Flash::MAX_FLASH_SIZE;
  }

  int start_sector = (start_address + FS_ALIGNMENT - 1) / FS_ALIGNMENT;
  int end_sector = (end_address + FS_ALIGNMENT - 1) / FS_ALIGNMENT;

  for (int n = start_sector; n < end_sector; n++) {
    uint32_t *sector_ptr = static_cast<uint32_t *>(Flash::readPointer(n * FS_ALIGNMENT));
    if (sector_ptr == nullptr) {
      return nullptr;
    }
    if (*sector_ptr == MAGIC_NUMBER) {
      return initializeFileSystem(sector_ptr);
    }
  }
  return nullptr;
}

std::shared_ptr<SPFS::Directory> SPFS::initializeFileSystem(void *address) {
  FileSystemHeader *header = static_cast<FileSystemHeader *>(address);
  if (header->magic != MAGIC_NUMBER) {
    printf("Invalid magic number: 0x%08X\n", header->magic);
    return nullptr;
  }

  if((header->version & (VERSION_MAJOR_MASK | VERSION_MINOR_MASK)) != (SPFS_VERSION & (VERSION_MAJOR_MASK | VERSION_MINOR_MASK))){
    return nullptr;
  }

  if(header->checksum == 0xFFFFFFFF) {
    auto size = header->size;

    if(!formatDisk(address, size)) {
      return nullptr;
    }

    return createNewFileSystem(address, size, "SPFS", "root");
  }
  
  if(header->checksum != calculateCRC32(header, sizeof(FileSystemHeader) - sizeof(header->checksum))) {
    return nullptr;
  }

  _fs_header = header;
  return std::make_shared<SPFS::DirectoryInternal>(this, nullptr, reinterpret_cast<const DirectoryHeader *>(address));
}

bool SPFS::formatDisk(const void *address, size_t size) {
  return Flash::erase(address, size) == 0;
}

std::shared_ptr<SPFS::Directory> SPFS::Directory::createDirectory(const std::string& name){
  auto new_dir = _fs->createDirectory(shared_from_this(), name);
  if(new_dir == nullptr){
    return nullptr;
  }

  std::vector<uint8_t> buffer(FS_BLOCK_SIZE);
  if(Flash::read(buffer, getHeader()) < (int)buffer.size()) {
    return nullptr;
  }

  DirectoryHeader *dirheader = reinterpret_cast<DirectoryHeader *>(buffer.data());

  auto contentHeaders = reinterpret_cast<DirectoryContentHeader *>(reinterpret_cast<uint8_t*>(dirheader) + (dirheader->name_size_content_offset >> 8));
  auto max_count = (int)(FS_BLOCK_SIZE - (dirheader->name_size_content_offset >> 8)) / sizeof(DirectoryContentHeader);

  int current = 0;
  while(contentHeaders[current].type != 0xFFFF && current < max_count) {
    current++;
  }

  if(current >= max_count) {
    return nullptr; // No space for new content
  }

  contentHeaders[current].type = 0xD1FF; // Directory type
  contentHeaders[current].block_offset = (int16_t)(((uintptr_t)new_dir->getHeader() - (uintptr_t)getHeader()) / FS_BLOCK_SIZE);

  if(Flash::write(buffer, getHeader()) < (int)buffer.size()) {
    return nullptr;
  }

  return new_dir;
}

std::shared_ptr<SPFS::DirectoryInternal> SPFS::createDirectory(const std::shared_ptr<SPFS::Directory> parent, const std::string& dir_name) {
  auto address = findFreeSpaceForDirectory();
  if(address == nullptr) {
    return nullptr;
  }
  return createDirectory(address, parent, dir_name);
}

std::shared_ptr<SPFS::DirectoryInternal> SPFS::createDirectory(const void* address, const std::shared_ptr<SPFS::Directory> parent, const std::string& dir_name) {
  if(dir_name.length() >= 200) {
    return nullptr; // Name too long
  }

  std::vector<uint8_t> buffer(FS_BLOCK_SIZE);
  if(Flash::read(buffer, address) < (int)buffer.size()) {
    return nullptr;
  }

  DirectoryHeader *dirheader = reinterpret_cast<DirectoryHeader *>(buffer.data());

  dirheader->magic = MAGIC_DIR_NUMBER;
  dirheader->name_size_content_offset = (uint16_t)(dir_name.length() & 0xFF) | ((sizeof(DirectoryHeader) + dir_name.length() + 1 + sizeof(DirectoryContentHeader)) & 0xFC) << 8;
  strncpy(reinterpret_cast<char*>(dirheader) + sizeof(DirectoryHeader), dir_name.c_str(), dir_name.length() + 1);
  dirheader->checksum = calculateCRC16(dirheader, sizeof(DirectoryHeader) - sizeof(dirheader->checksum));

  if(Flash::write(buffer, address) < (int)buffer.size()) {
    return nullptr;
  }

  return std::make_shared<SPFS::DirectoryInternal>(this, parent, reinterpret_cast<const DirectoryHeader *>(address));
}

const std::string SPFS::Directory::getName() const {
  const char* name_ptr = reinterpret_cast<const char*>(_header) + sizeof(SPFS::DirectoryHeader);
  return std::string(name_ptr, _header->name_size_content_offset & 0x00FF);
}

std::shared_ptr<SPFS::Directory> SPFS::createNewFileSystem(const void *address, size_t size, const std::string& fs_name, const std::string& root_dir_name) {
  std::vector<uint8_t> buffer(FS_BLOCK_SIZE);
  if(Flash::read(buffer, address) < (int)buffer.size()) {
    return nullptr;
  }

  FileSystemHeader *newheader = reinterpret_cast<FileSystemHeader *>(buffer.data());

  if(newheader->magic != MAGIC_NUMBER || newheader->version != SPFS_VERSION || newheader->size != size) {
    if(newheader->magic == 0xFFFFFFFF && newheader->version == 0xFFFFFFFF && newheader->size == 0xFFFFFFFF) {
      newheader->magic = MAGIC_NUMBER;
      newheader->version = SPFS_VERSION;
      newheader->size = size;
    }else{
      return nullptr;
    }
  }
  newheader->block_and_page_size = (FS_BLOCK_SIZE & 0xFFFF) | ((FS_ALIGNMENT & 0xFFFF) << 16);
  newheader->meta_offset = sizeof(FileSystemHeader);
  newheader->checksum = calculateCRC32(newheader, sizeof(FileSystemHeader) - sizeof(newheader->checksum));

  FileSystemMetadata *fsmeta = reinterpret_cast<FileSystemMetadata *>(buffer.data() + newheader->meta_offset);
  fsmeta->magic = MAGIC_FS_METADATA_NUMBER;
  fsmeta->name_size = (uint16_t)(fs_name.length() & 0xFF);

  std::shared_ptr<SPFS::Directory> root_dir = nullptr;
  fsmeta->root_directory_block = 0;
  while(root_dir == nullptr && fsmeta->root_directory_block < (size / FS_BLOCK_SIZE)) {
    fsmeta->root_directory_block++;
    root_dir = createDirectory(reinterpret_cast<const uint8_t*>(address) + FS_BLOCK_SIZE, nullptr, root_dir_name);
  }
  
  if(root_dir == nullptr) {
    return nullptr;
  }

  size_t max_name_length = FS_BLOCK_SIZE - (sizeof(FileSystemMetadata) + sizeof(FileSystemHeader)) - 1;
  if(fs_name.length() < (size_t)max_name_length) {
    max_name_length = fs_name.length();
  }
  strncpy(reinterpret_cast<char*>(fsmeta) + sizeof(FileSystemMetadata), fs_name.c_str(), max_name_length + 1);
  fsmeta->checksum = calculateCRC16(fsmeta, sizeof(FileSystemMetadata) - sizeof(fsmeta->checksum));

  if(Flash::write(buffer, address) < (int)buffer.size()) {
    return nullptr;
  }

  _fs_header = reinterpret_cast<const FileSystemHeader *>(address);
  return root_dir;
}

const SPFS::DirectoryHeader* SPFS::findFreeSpaceForDirectory(){
  return reinterpret_cast<const SPFS::DirectoryHeader*>(SPFS::findFreeSpace(sizeof(SPFS::DirectoryHeader)));
}
const SPFS::FileHeader* SPFS::findFreeSpaceForFile(size_t name_size){
  return reinterpret_cast<const SPFS::FileHeader*>(SPFS::findFreeSpace(sizeof(SPFS::FileHeader) + name_size));
}
const SPFS::FileContentHeader* SPFS::findFreeSpaceForFileContent(size_t content_size){
  return reinterpret_cast<const SPFS::FileContentHeader*>(SPFS::findFreeSpace(sizeof(SPFS::FileContentHeader) + content_size));
}
const void* SPFS::findFreeSpace(size_t size){
  if(_start_search_address == nullptr){
    _start_search_address = reinterpret_cast<const uint8_t*>(_fs_header) + FS_BLOCK_SIZE;
  }
  return findFreeSpace(_start_search_address, size);
}
const void* SPFS::findFreeSpace(const uint8_t* start_search, size_t size){
  const uint8_t* end_address = reinterpret_cast<const uint8_t*>(_fs_header) + _fs_header->size;
  while(reinterpret_cast<const uint32_t*>(start_search)[0] != 0xFFFFFFFF){
    start_search += FS_BLOCK_SIZE;
    if(start_search >= end_address){
      return nullptr;
    }
  }
  return start_search;
}

uint32_t SPFS::calculateCRC32(const void *address, size_t size) {
  uint32_t crc = 0xFFFFFFFFu;
  const uint32_t polynomial = 0xEDB88320u;
  const uint8_t *data = static_cast<const uint8_t *>(address);
  for (size_t i = 0; i < size; ++i) {
    crc ^= data[i];
    for (int j = 0; j < 8; ++j) {
      if (crc & 1u)
        crc = (crc >> 1) ^ polynomial;
      else
        crc >>= 1;
    }
  }
  return crc ^ 0xFFFFFFFFu;
}

uint16_t SPFS::calculateCRC16(const void *address, size_t size) {
  uint16_t crc = 0xFFFFu;
  const uint16_t polynomial = 0xA001u;
  const uint8_t *data = static_cast<const uint8_t *>(address);
  for (size_t i = 0; i < size; ++i) {
    crc ^= data[i];
    for (int j = 0; j < 8; ++j) {
      if (crc & 1u)
        crc = (crc >> 1) ^ polynomial;
      else
        crc >>= 1;
    }
  }
  return crc ^ 0xFFFFFFFFu;
}

