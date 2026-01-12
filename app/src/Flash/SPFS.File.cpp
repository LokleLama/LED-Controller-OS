#include "SPFS.h"
#include "flash.h"
#include <cstring>

SPFS::File::File(std::shared_ptr<Directory> parent, const std::string& name) : File(nullptr, parent, nullptr) {
  auto file = parent->openFile(name);
  if(file != nullptr){
    _fs = file->_fs;
    _header = file->_header;
    FindCurrentContentHeader();
  }else{
    file = parent->createFile(name);
    if(file != nullptr){
      _fs = file->_fs;
      _header = file->_header;
    }
  }
}

void SPFS::File::FindCurrentContentHeader() {
  if(_content_header != nullptr) {
    return;
  }
  _content_version = 0;
  uint16_t next_block = getMetadataHeader()->content_block;
  const void* content_address = reinterpret_cast<const void*>(_header);
  while (next_block != 0xFFFF) {
    _content_version++;
    _content_header = _fs->calculateContentHeaderAddress(content_address, next_block);
    content_address = _content_header;
    next_block = _content_header->next_version;
  }
}

bool SPFS::File::write(const std::string& data){
  return write(reinterpret_cast<const uint8_t*>(data.data()), data.length() + 1);
}
bool SPFS::File::write(const std::vector<uint8_t>& data){
  return write(data.data(), data.size());
}
bool SPFS::File::write(const uint8_t* data, size_t size) {
  if(!allocateContenSize(size)) {
    return false;
  }

  if(!append(data, size)) {
    return false;
  }

  if(!finishContent()) {
    return false;
  }
  return true;
}

bool SPFS::File::allocateContenSize(size_t size) {
  if(_current_content_header != nullptr) {
    return false; // Content already allocated
  }

  // Find free space for new file content
  _current_content_header = _fs->findFreeSpaceForFileContent(size);
  if(_current_content_header == nullptr) {
    return false;
  }

  uint16_t content_block_offset = 0xFFFF;
  if(_content_header != nullptr) {
    content_block_offset = _fs->calculateContentBlockOffset(_content_header, _current_content_header);
  }else{
    content_block_offset = _fs->calculateContentBlockOffset(_header, _current_content_header);
  }
  if(content_block_offset == 0xFFFF) {
    return false; // Invalid content block: difference is too big
  }

  std::vector<uint8_t> buffer(FS_BLOCK_SIZE, 0xFF);

  FileContentHeader *contentheader = reinterpret_cast<FileContentHeader *>(buffer.data());
  contentheader->block.magic = MAGIC_FILE_CONTENT_NUMBER;
  contentheader->data_offset = ((sizeof(FileContentHeader) + 7) & 0x00F8) | 0xFF00; // Align to 4 bytes
  // Do not set size, block size and checksum yet
  contentheader->next_partition = 0xFFFF; // No next content
  contentheader->next_version = 0xFFFF; // No next content

  if(Flash::write(buffer, _current_content_header) < (int)buffer.size()) {
    _current_content_header = nullptr;
    return false;
  }
  _append_position = (contentheader->data_offset & 0x00FF);
  return true;
}

bool SPFS::File::append(const std::string& data){
  return append(reinterpret_cast<const uint8_t*>(data.data()), data.length() + 1);
}
bool SPFS::File::append(const std::vector<uint8_t>& data){
  return append(data.data(), data.size());
}
bool SPFS::File::append(const uint8_t* data, size_t size) {
  if(_current_content_header == nullptr) {
    return false; // No allocated content
  }

  std::vector<uint8_t> buffer(FS_BLOCK_SIZE, 0xFF);

  auto pointer = reinterpret_cast<const uint8_t*>(_current_content_header) + (_append_position & ~(FS_BLOCK_SIZE - 1));

  size_t current_pos = 0;
  while(current_pos < size) {
    if(Flash::read(buffer, pointer) != (int)buffer.size()) {
      return false;
    }

    int position_within_block = _append_position & (FS_BLOCK_SIZE - 1);

    int to_copy = size - current_pos;
    if(to_copy > FS_BLOCK_SIZE - position_within_block) {
      to_copy = FS_BLOCK_SIZE - position_within_block;
    }
    memcpy(buffer.data() + position_within_block, data + current_pos, to_copy);
    if(to_copy < FS_BLOCK_SIZE - position_within_block) {
      // Fill the rest with 0xFF
      memset(buffer.data() + position_within_block + to_copy, 0xFF, FS_BLOCK_SIZE - position_within_block - to_copy);
    }
    if(Flash::write(buffer, pointer) < (int)buffer.size()) {
      return false;
    }
    current_pos += to_copy;
    _append_position += to_copy;
    pointer = pointer + FS_BLOCK_SIZE;
  }
  return true;
}

bool SPFS::File::finishContent() {
  if(_current_content_header == nullptr) {
    return false; // No allocated content
  }

  std::vector<uint8_t> buffer(FS_BLOCK_SIZE, 0xFF);
  if(Flash::read(buffer, _current_content_header) < (int)buffer.size()) {
    return false;
  }

  FileContentHeader *contentheader = reinterpret_cast<FileContentHeader *>(buffer.data());
  contentheader->block.size = (uint16_t)((_append_position + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE);
  contentheader->size = _append_position - (contentheader->data_offset & 0x00FF);
  contentheader->checksum = _fs->calculateCRC16(contentheader, sizeof(FileContentHeader) - sizeof(contentheader->checksum) - sizeof(contentheader->next_partition) - sizeof(contentheader->next_version));

  if(Flash::write(buffer, _current_content_header) < (int)buffer.size()) {
    return false;
  }

  uint16_t content_block_offset = 0xFFFF;
  if(_content_header != nullptr) {
    content_block_offset = _fs->calculateContentBlockOffset(_content_header, _current_content_header);
  }else{
    content_block_offset = _fs->calculateContentBlockOffset(_header, _current_content_header);
  }
  if(content_block_offset == 0xFFFF) {
    return false; // Invalid content block: difference is too big
  }

  if(_content_header != nullptr) {
    if(Flash::read(buffer, _content_header) < (int)buffer.size()) {
      return false;
    }
    FileContentHeader *prev_contentheader = reinterpret_cast<FileContentHeader *>(buffer.data());
    prev_contentheader->next_version = content_block_offset;
    if(Flash::write(buffer, _content_header) < (int)buffer.size()) {
      return false;
    }
  }else{
    if(Flash::read(buffer, _header) < (int)buffer.size()) {
      return false;
    }
    FileMetadataHeader *filemeta = reinterpret_cast<FileMetadataHeader *>(buffer.data() + (_header->name_size_meta_offset >> 8));
    filemeta->content_block = content_block_offset;
    if(Flash::write(buffer, _header) < (int)buffer.size()) {
      return false;
    }
  }
  _content_header = _current_content_header;
  _content_version++;
  _current_content_header = nullptr;
  return true;
}
