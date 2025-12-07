#include "SPFS.h"
#include "flash.h"
#include <cstring>

const std::string SPFS::ReadOnlyFile::getName() const {
  const char* name_ptr = reinterpret_cast<const char*>(_header) + sizeof(SPFS::FileHeader);
  return std::string(name_ptr, _header->name_size_meta_offset & 0x00FF);
}

size_t SPFS::ReadOnlyFile::getFileSize() const {
  if(_content_header == nullptr) {
    return 0;
  }
  return _content_header->size;
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