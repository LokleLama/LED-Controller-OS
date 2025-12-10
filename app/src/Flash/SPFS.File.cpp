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
  while (next_block != 0xFFFF) {
    _content_version++;
    const uint8_t* content_address = reinterpret_cast<const uint8_t*>(_header) + next_block * SPFS::FS_BLOCK_SIZE;
    _content_header = reinterpret_cast<const FileContentHeader*>(content_address);
    next_block = _content_header->next;
  }
}

size_t SPFS::File::getFileSizeOnDisk() const {
  size_t total_size_on_disk = _header->block.size;
  uint16_t next_block = getMetadataHeader()->content_block;
  while (next_block != 0xFFFF) {
    const uint8_t* content_address = reinterpret_cast<const uint8_t*>(_header) + next_block * SPFS::FS_BLOCK_SIZE;
    const FileContentHeader* content_header = reinterpret_cast<const FileContentHeader*>(content_address);
    total_size_on_disk += content_header->block.size;
    next_block = content_header->next;
  }
  return total_size_on_disk * SPFS::FS_BLOCK_SIZE;
}

bool SPFS::File::write(std::string data){
  return write(reinterpret_cast<const uint8_t*>(data.data()), data.length() + 1);
}
bool SPFS::File::write(std::vector<uint8_t> data){
  return write(data.data(), data.size());
}

bool SPFS::File::write(const uint8_t* data, size_t size) {
  // Find free space for new file content
  auto address = _fs->findFreeSpaceForFileContent(size);
  if(address == nullptr) {
    return false;
  }

  std::vector<uint8_t> buffer(FS_BLOCK_SIZE, 0xFF);

  FileContentHeader *contentheader = reinterpret_cast<FileContentHeader *>(buffer.data());
  contentheader->block.magic = MAGIC_FILE_CONTENT_NUMBER;
  contentheader->data_offset = ((sizeof(FileContentHeader) + 7) & 0x00F8) | 0xFF00; // Align to 4 bytes
  contentheader->block.size = (uint16_t)(((contentheader->data_offset & 0x00FF) + size + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE);
  contentheader->size = size;
  contentheader->checksum = _fs->calculateCRC16(contentheader, sizeof(FileContentHeader) - sizeof(contentheader->checksum)- sizeof(contentheader->next));
  contentheader->next = 0xFFFF; // No next content

  size_t current_pos = 0;

  int to_copy = size;
  if(to_copy > FS_BLOCK_SIZE - (contentheader->data_offset & 0x00FF)) {
    to_copy = FS_BLOCK_SIZE - (contentheader->data_offset & 0x00FF);
  }
  memcpy(buffer.data() + (contentheader->data_offset & 0x00FF), data, to_copy);
  current_pos += to_copy;
  if(Flash::write(buffer, address) < (int)buffer.size()) {
    return false;
  }

  auto pointer = reinterpret_cast<const uint8_t*>(address);
  while(current_pos < size) {
    pointer = pointer + FS_BLOCK_SIZE;

    to_copy = size - current_pos;
    if(to_copy > FS_BLOCK_SIZE) {
      to_copy = FS_BLOCK_SIZE;
    }
    memcpy(buffer.data(), data + current_pos, to_copy);
    if(to_copy < FS_BLOCK_SIZE) {
      memset(buffer.data() + to_copy, 0xFF, FS_BLOCK_SIZE - to_copy);
    }
    current_pos += to_copy;
    if(Flash::write(buffer, pointer) < (int)buffer.size()) {
      return false;
    }
  }

  if(_content_header != nullptr) {
    if(Flash::read(buffer, _content_header) < (int)buffer.size()) {
      return false;
    }
    FileContentHeader *prev_contentheader = reinterpret_cast<FileContentHeader *>(buffer.data());
    prev_contentheader->next = (uint16_t)((reinterpret_cast<const uint8_t*>(address) - reinterpret_cast<const uint8_t*>(_header)) / FS_BLOCK_SIZE);
    if(Flash::write(buffer, _content_header) < (int)buffer.size()) {
      return false;
    }
  }else{
    if(Flash::read(buffer, _header) < (int)buffer.size()) {
      return false;
    }
    FileMetadataHeader *filemeta = reinterpret_cast<FileMetadataHeader *>(buffer.data() + (_header->name_size_meta_offset >> 8));
    filemeta->content_block = (uint16_t)((reinterpret_cast<const uint8_t*>(address) - reinterpret_cast<const uint8_t*>(_header)) / SPFS::FS_BLOCK_SIZE);
    if(Flash::write(buffer, _header) < (int)buffer.size()) {
      return false;
    }
  }
  _content_header = reinterpret_cast<const FileContentHeader*>(address);
  _content_version++;

  return true;
}
