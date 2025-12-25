#include "SPFS.h"
#include "flash.h"
#include <cstring>

const std::string SPFS::ReadOnlyFile::getName() const {
  const char* name_ptr = reinterpret_cast<const char*>(_header) + sizeof(SPFS::FileHeader);
  return std::string(name_ptr, _header->name_size_meta_offset & 0x00FF);
}

size_t SPFS::ReadOnlyFile::getSize() const {
  if(_content_header == nullptr) {
    return 0;
  }
  return _content_header->size;
}

size_t SPFS::ReadOnlyFile::getSizeOnDisk() const {
  size_t total_size_on_disk = _header->block.size;
  uint16_t next_block = getMetadataHeader()->content_block;
  const void* content_address = reinterpret_cast<const void*>(_header);
  while (next_block != 0xFFFF) {
    auto content_header = _fs->calculateContentHeaderAddress(content_address, next_block);
    content_address = content_header;
    total_size_on_disk += content_header->block.size;
    next_block = content_header->next_version;
  }
  return total_size_on_disk * SPFS::FS_BLOCK_SIZE;
}

const uint8_t* SPFS::ReadOnlyFile::getMemoryMappedAddress() const {
  if(_content_header == nullptr) {
    return nullptr;
  }
  return reinterpret_cast<const uint8_t*>(_content_header) + (_content_header->data_offset & 0x00FF);
}
std::string SPFS::ReadOnlyFile::readAsString() const {
  if(_content_header == nullptr) {
    return "";
  }
  const char * data_ptr = reinterpret_cast<const char*>(getMemoryMappedAddress());
  return std::string(data_ptr, _content_header->size);
}
std::vector<uint8_t> SPFS::ReadOnlyFile::readAsVector() const {
  if(_content_header == nullptr) {
    return std::vector<uint8_t>();
  }
  std::vector<uint8_t> data_vector(_content_header->size);
  const uint8_t * data_ptr = getMemoryMappedAddress();
  memcpy(data_vector.data(), data_ptr, _content_header->size);
  return data_vector;
}
std::vector<uint8_t> SPFS::ReadOnlyFile::readBytes(size_t offset, size_t size) const {
  if(_content_header == nullptr || offset >= _content_header->size) {
    return std::vector<uint8_t>();
  }
  if(offset + size > _content_header->size) {
    size = _content_header->size - offset;
  }
  std::vector<uint8_t> data_vector(size);
  const uint8_t * data_ptr = getMemoryMappedAddress() + offset;
  memcpy(data_vector.data(), data_ptr, size);
  return data_vector;
}

std::shared_ptr<SPFS::ReadOnlyFile> SPFS::ReadOnlyFile::openVersion(size_t version) const{
  if(version >= _content_version) {
    return nullptr;
  }
  if(version == 0) {
    return std::make_shared<SPFS::ReadOnlyFileInternal>(_fs, _parent, _header, nullptr, 0);
  }

  const FileContentHeader* content_header = nullptr;
  size_t current_version = 0;
  uint16_t next_block = getMetadataHeader()->content_block;
  const void* content_address = reinterpret_cast<const void*>(_header);
  while (next_block != 0xFFFF) {
    content_header = _fs->calculateContentHeaderAddress(content_address, next_block);
    content_address = content_header;
    current_version++;
    if(current_version == version) {
      break;
    }
    next_block = content_header->next_version;
  }
  return std::make_shared<SPFS::ReadOnlyFileInternal>(_fs, _parent, _header, content_header, version);
}