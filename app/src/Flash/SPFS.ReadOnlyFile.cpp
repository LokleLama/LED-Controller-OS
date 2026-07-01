#include "SPFS.Internal.h"
#include "flash.h"
#include <cstring>

namespace {
constexpr uint16_t kInvalidBlockOffset = 0xFFFF;
}

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
  if(next_block == kInvalidBlockOffset) {
    return total_size_on_disk * SPFS::FS_BLOCK_SIZE;
  }

  auto content_header = calculateContentHeaderAddress(_header, next_block);
  total_size_on_disk += content_header->block.size;
  next_block = content_header->next_version;
  
  while (next_block != kInvalidBlockOffset) {
    content_header = calculateContentHeaderAddress(content_header, next_block);
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
    return {};
  }
  const char * data_ptr = reinterpret_cast<const char*>(getMemoryMappedAddress());
  return std::string(data_ptr, _content_header->size);
}
std::vector<uint8_t> SPFS::ReadOnlyFile::readAsVector() const {
  if(_content_header == nullptr) {
    return {};
  }
  std::vector<uint8_t> data_vector(_content_header->size);
  const uint8_t * data_ptr = getMemoryMappedAddress();
  memcpy(data_vector.data(), data_ptr, _content_header->size);
  return data_vector;
}
std::vector<uint8_t> SPFS::ReadOnlyFile::readBytes(size_t offset, size_t size) const {
  if(_content_header == nullptr || offset >= _content_header->size) {
    return {};
  }
  if(size == (size_t)-1 || offset + size > _content_header->size) {
    size = _content_header->size - offset;
  }
  std::vector<uint8_t> data_vector(size);
  const uint8_t * data_ptr = getMemoryMappedAddress() + offset;
  memcpy(data_vector.data(), data_ptr, size);
  return data_vector;
}

std::shared_ptr<SPFS::ReadOnlyFile> SPFS::ReadOnlyFile::openVersion(size_t version) {
  if(version > _content_version) {
    return nullptr;
  }
  if(version > _content_version) {
    return shared_from_this();
  }
  if(version == 0) {
    return std::make_shared<SPFS::ReadOnlyFileInternal>(_fs, _parent, _header, nullptr, 0);
  }

  size_t current_version = 0;
  const FileContentHeader* content_header = getContentHeader(_header);
  if(content_header == nullptr) {
    return std::make_shared<SPFS::ReadOnlyFileInternal>(_fs, _parent, _header, nullptr, current_version);
  }
  current_version++;

  for( ; current_version < version && content_header != nullptr; current_version++) {
    content_header = getContentHeader(content_header);
  }
  return std::make_shared<SPFS::ReadOnlyFileInternal>(_fs, _parent, _header, content_header, version);
}

std::unique_ptr<std::istream> SPFS::ReadOnlyFile::getInputStream() const {
  if(_content_header == nullptr) {
    // Return an empty stream for empty files
    return std::make_unique<std::istream>(new SPFS::ReadOnlyFileStreamBuf(nullptr, 0));  //Memory Leak: The streambuf is allocated with new but not deleted. Consider using a smart pointer or managing the lifetime of the streambuf to avoid memory leaks.
  }
  
  const uint8_t* data = getMemoryMappedAddress();
  size_t size = _content_header->size;
  
  // Create a custom streambuf and wrap it in an istream
  auto* buf = new SPFS::ReadOnlyFileStreamBuf(data, size); //Memory Leak: The streambuf is allocated with new but not deleted. Consider using a smart pointer or managing the lifetime of the streambuf to avoid memory leaks.
  auto stream = std::make_unique<std::istream>(buf);
  
  // The istream will take ownership of the streambuf and delete it when done
  return stream;
}

const SPFS::FileContentHeader* SPFS::ReadOnlyFile::FindNewestContentHeader(const SPFS::FileHeader* header, size_t& version_counter) const {
  version_counter = 0;
  uint16_t next_block = getMetadataHeader(header)->content_block;
  if(next_block == kInvalidBlockOffset) {
    return nullptr; // No content blocks
  }
  auto content_header = calculateContentHeaderAddress(header, next_block);
  if(content_header == nullptr) {
    return nullptr; // Invalid content header
  }
  if(content_header->size == kInvalidBlockOffset) {
    return nullptr; // Invalid size
  }
  version_counter++;
  return FindNewestContentHeader(content_header, version_counter);
}

const SPFS::FileContentHeader* SPFS::ReadOnlyFile::FindNewestContentHeader(const SPFS::FileContentHeader* content_header) const {
  size_t version_counter = 0;
  return FindNewestContentHeader(content_header, version_counter);
}

const SPFS::FileContentHeader* SPFS::ReadOnlyFile::FindNewestContentHeader(const SPFS::FileContentHeader* content_header, size_t& version_counter) const {
  uint16_t next_block = content_header->next_version;
  while (next_block != kInvalidBlockOffset) {
    version_counter++;
    const SPFS::FileContentHeader* next_header = calculateContentHeaderAddress(content_header, next_block);
    if(next_header == nullptr) {
      return content_header; // Invalid next header, return the last valid one
    }
    if(next_header->block.magic != MAGIC_FILE_CONTENT_NUMBER) {
      return content_header; // Invalid magic number, return the last valid one
    }
    if(next_header->size == kInvalidBlockOffset) {
      return content_header; // Invalid size, return the last valid one
    }
    content_header = next_header;
    next_block = content_header->next_version;
  }
  return content_header;
}

bool SPFS::ReadOnlyFile::createTag(const std::string& tag_description){
  if(_content_header == nullptr) {
    return false; // No content to tag
  }

  if(_content_header->tag_metadata_block != kInvalidBlockOffset) {
    return false; // Tag already exists
  }
  
  auto tag_header_address = _fs->findFreeSpaceForFileContentTag(tag_description.length());
  if(tag_header_address == nullptr) {
    return false; // No space available for tag
  }

  const uint8_t* data = reinterpret_cast<const uint8_t*>(tag_description.data());
  size_t current_pos = 0;

  std::vector<uint8_t> buffer(FS_BLOCK_SIZE, 0xFF);

  FileContentTagHeader *tag_header = reinterpret_cast<FileContentTagHeader *>(buffer.data());
  tag_header->block.magic = MAGIC_FILE_TAG_NUMBER;
  tag_header->tag_size = tag_description.length();
  
  size_t data_offset = (sizeof(FileContentTagHeader) + 3) & 0x00FC; // Align to 4 bytes

  tag_header->data_offset = ((uint16_t)data_offset) | 0xFF00;
  tag_header->block.size = (uint16_t)((data_offset + tag_description.length() + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE);
  tag_header->checksum = _fs->calculateCRC16(tag_header, sizeof(FileContentTagHeader) - sizeof(tag_header->checksum));

  size_t to_copy = tag_description.length();
  if(to_copy > FS_BLOCK_SIZE - data_offset) {
    to_copy = FS_BLOCK_SIZE - data_offset;
  }
  memcpy(buffer.data() + data_offset, data, to_copy);
  current_pos += to_copy;

  if(Flash::write(buffer, tag_header_address) < (int)buffer.size()) {
    return false;
  }

  const uint8_t* pointer = reinterpret_cast<const uint8_t*>(tag_header_address) + FS_BLOCK_SIZE;
  while(current_pos < tag_description.length()) {
    size_t to_copy = tag_description.length() - current_pos;
    if(to_copy > FS_BLOCK_SIZE) {
      to_copy = FS_BLOCK_SIZE;
    }else{
      memset(buffer.data() + to_copy, 0xFF, FS_BLOCK_SIZE - to_copy);
    }
    memcpy(buffer.data(), data + current_pos, to_copy);
    if(Flash::write(buffer, pointer) < (int)buffer.size()) {
      return false;
    }
    current_pos += to_copy;
    pointer += FS_BLOCK_SIZE;
  }

  if(Flash::read(buffer, _content_header) != (int)buffer.size()) {
    return false;
  }
  FileContentHeader *contentheader = reinterpret_cast<FileContentHeader *>(buffer.data());
  contentheader->tag_metadata_block = _fs->calculateContentTagBlockOffset(_content_header, tag_header_address);
  if(Flash::write(buffer, _content_header) < (int)buffer.size()) {
    return false;
  }
  return true;
}

std::string SPFS::ReadOnlyFile::readTag() const{
  if(_content_header == nullptr || _content_header->tag_metadata_block == kInvalidBlockOffset || _content_header->tag_metadata_block == 0) {
    return {}; // No content or no tag
  }

  auto tag_header = calculateContentTagHeaderAddress(_content_header, _content_header->tag_metadata_block);
  if(tag_header == nullptr || tag_header->block.magic != MAGIC_FILE_TAG_NUMBER) {
    return {}; // Invalid tag header
  }

  const uint8_t* data_ptr = reinterpret_cast<const uint8_t*>(tag_header) + (tag_header->data_offset & 0x00FF);
  return std::string(reinterpret_cast<const char*>(data_ptr), tag_header->tag_size);
}


bool SPFS::ReadOnlyFile::deleteTag(){
  if(_content_header == nullptr || _content_header->tag_metadata_block == kInvalidBlockOffset || _content_header->tag_metadata_block == 0) {
    return false; // No content or no tag
  }

  std::vector<uint8_t> buffer(FS_BLOCK_SIZE);
  if(Flash::read(buffer, _content_header) != (int)buffer.size()) {
    return false;
  }
  FileContentHeader *contentheader = reinterpret_cast<FileContentHeader *>(buffer.data());
  contentheader->tag_metadata_block = 0;
  if(Flash::write(buffer, _content_header) < (int)buffer.size()) {
    return false;
  }
  return true;
}


const SPFS::FileContentHeader* SPFS::ReadOnlyFile::getContentHeader(const SPFS::FileHeader* header) const {
  auto metadata_header = getMetadataHeader(header);
  if(metadata_header == nullptr) {
    return nullptr;
  }
  uint16_t next_block = metadata_header->content_block;
  if(next_block == kInvalidBlockOffset) {
    return nullptr; // No content blocks
  }
  return calculateContentHeaderAddress(header, next_block);
}

const SPFS::FileContentHeader* SPFS::ReadOnlyFile::getContentHeader(const SPFS::FileContentHeader* content_header) const {
  if(content_header == nullptr) {
    return nullptr;
  }
  uint16_t next_block = content_header->next_version;
  if(next_block == kInvalidBlockOffset) {
    return nullptr; // No content blocks
  }
  return calculateContentHeaderAddress(content_header, next_block);
}

const SPFS::FileContentHeader* SPFS::ReadOnlyFile::calculateContentHeaderAddress(const SPFS::FileHeader* reference_address, uint16_t content_block_offset) const {
  if(content_block_offset == kInvalidBlockOffset) {
    return nullptr;
  }
  uintptr_t content_address = reinterpret_cast<uintptr_t>(reference_address);
  if((content_block_offset & 0x8000) == 0){
    content_address += (content_block_offset & 0x7FFF) * FS_BLOCK_SIZE;
  }else{
    content_address -= (content_block_offset & 0x7FFF) * FS_BLOCK_SIZE;
  }
  return reinterpret_cast<const FileContentHeader*>(content_address);
}

const SPFS::FileContentHeader* SPFS::ReadOnlyFile::calculateContentHeaderAddress(const SPFS::FileContentHeader* reference_address, uint16_t content_block_offset) const {
  if(content_block_offset == kInvalidBlockOffset) {
    return nullptr;
  }
  uintptr_t content_address = reinterpret_cast<uintptr_t>(reference_address);
  if((content_block_offset & 0x8000) == 0){
    content_address += (content_block_offset & 0x7FFF) * FS_BLOCK_SIZE;
  }else{
    content_address -= (content_block_offset & 0x7FFF) * FS_BLOCK_SIZE;
  }
  return reinterpret_cast<const FileContentHeader*>(content_address);
}

const SPFS::FileContentTagHeader* SPFS::ReadOnlyFile::calculateContentTagHeaderAddress(const SPFS::FileContentHeader* reference_address, uint16_t content_block_offset) const {
  if(content_block_offset == kInvalidBlockOffset) {
    return nullptr;
  }
  uintptr_t content_address = reinterpret_cast<uintptr_t>(reference_address);
  if((content_block_offset & 0x8000) == 0){
    content_address += (content_block_offset & 0x7FFF) * FS_BLOCK_SIZE;
  }else{
    content_address -= (content_block_offset & 0x7FFF) * FS_BLOCK_SIZE;
  }
  return reinterpret_cast<const FileContentTagHeader*>(content_address);
}
