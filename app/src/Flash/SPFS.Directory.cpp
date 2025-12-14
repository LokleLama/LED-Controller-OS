#include "SPFS.h"
#include "flash.h"
#include <cstring>

SPFS::Directory::Directory(std::shared_ptr<Directory> parent, const std::string& name) : Directory(nullptr, parent, nullptr) {
  auto dir = parent->openSubdirectory(name);
  if(dir != nullptr){
    _fs = dir->_fs;
    _header = dir->_header;
  }else{
    dir = parent->createDirectory(name);
    if(dir != nullptr){
      _fs = dir->_fs;
      _header = dir->_header;
    }
  }
}

size_t SPFS::Directory::getSizeOnDisk() const {
  size_t total_size_on_disk = _header->block.size;
  uint16_t next_block = getMetadataHeader()->next;
  while (next_block != 0xFFFF) {
    // not Yet implemented
    //total_size_on_disk += content_header->block.size;
    next_block = 0xFFFF;
  }
  return total_size_on_disk * SPFS::FS_BLOCK_SIZE;
}

std::vector<std::shared_ptr<SPFS::Directory>> SPFS::Directory::getSubdirectories() {
  std::vector<std::shared_ptr<SPFS::Directory>> subdirs;

  auto contentHeaders = getContentHeaders();
  auto max_count = getMaxContentCount();

  for(int i = 0; i < max_count; i++) {
    if(contentHeaders[i].type == MAGIC_SUBDIRMARKER) { // Directory type
      auto dir_address = reinterpret_cast<const uint8_t*>(getHeader()) + contentHeaders[i].block_offset * FS_BLOCK_SIZE;
      auto subdir = _fs->openDirectory(dir_address, shared_from_this());
      if(subdir != nullptr) {
        subdirs.push_back(subdir);
      }
    }else if(contentHeaders[i].type == MAGIC_ENDMARKER) {
      break; // End of content
    }
  }

  return subdirs;
}

std::vector<std::shared_ptr<SPFS::File>> SPFS::Directory::getFiles() {
  std::vector<std::shared_ptr<SPFS::File>> files;

  auto contentHeaders = getContentHeaders();
  auto max_count = getMaxContentCount();

  for(int i = 0; i < max_count; i++) {
    if(contentHeaders[i].type == MAGIC_FILEMARKER) { // Directory type
      auto file_address = reinterpret_cast<const uint8_t*>(getHeader()) + contentHeaders[i].block_offset * FS_BLOCK_SIZE;
      auto file = _fs->openFile(file_address, shared_from_this());
      if(file != nullptr) {
        files.push_back(file);
      }
    }else if(contentHeaders[i].type == MAGIC_ENDMARKER) {
      break; // End of content
    }
  }

  return files;
}

const std::string SPFS::Directory::getName() const {
  const char* name_ptr = reinterpret_cast<const char*>(_header) + sizeof(SPFS::DirectoryHeader);
  return std::string(name_ptr, _header->name_size_meta_offset & 0x00FF);
}

bool SPFS::Directory::addContent(uint16_t type, uintptr_t content_address){
  std::vector<uint8_t> buffer(FS_BLOCK_SIZE);
  if(Flash::read(buffer, getHeader()) < (int)buffer.size()) {
    return false;
  }

  DirectoryHeader *dirheader = reinterpret_cast<DirectoryHeader *>(buffer.data());

  DirectoryMetadataHeader *dirmeta = reinterpret_cast<DirectoryMetadataHeader *>(buffer.data() + (dirheader->name_size_meta_offset >> 8));
  auto contentHeaders = dirmeta->content;
  auto max_count = getMaxContentCount();

  uint16_t content_block_offset = (uint16_t)((content_address - (uintptr_t)getHeader()) / FS_BLOCK_SIZE);

  int current = 0;
  while(contentHeaders[current].type != 0xFFFF && current < max_count) {
    if(contentHeaders[current].block_offset == content_block_offset) {
      return false; // Content already exists
    }
    current++;
  }

  if(current >= max_count) {
    //TODO: Implement directory extension
    return false; // No space for new content
  }

  contentHeaders[current].type = type; // Directory type
  contentHeaders[current].block_offset = content_block_offset;

  if(Flash::write(buffer, getHeader()) < (int)buffer.size()) {
    return false;
  }
  return true;
}

bool SPFS::Directory::removeContent(uintptr_t content_address){
  std::vector<uint8_t> buffer(FS_BLOCK_SIZE);
  if(Flash::read(buffer, getHeader()) < (int)buffer.size()) {
    return false;
  }

  DirectoryHeader *dirheader = reinterpret_cast<DirectoryHeader *>(buffer.data());

  DirectoryMetadataHeader *dirmeta = reinterpret_cast<DirectoryMetadataHeader *>(buffer.data() + (dirheader->name_size_meta_offset >> 8));
  auto contentHeaders = dirmeta->content;
  auto max_count = getMaxContentCount();

  int current = 0;
  int16_t target_block = (int16_t)((content_address - (uintptr_t)getHeader()) / FS_BLOCK_SIZE);
  while(current < max_count && contentHeaders[current].type != 0xFFFF) {
    if(contentHeaders[current].block_offset == target_block) {
      contentHeaders[current].type &= 0x0FFF; // Mark as deleted
      if(Flash::write(buffer, getHeader()) < (int)buffer.size()) {
        return false;
      }
      return true;
    }
    current++;
  }

  return false; // Content not found
}

bool SPFS::Directory::addContent(std::shared_ptr<SPFS::Directory> dir){
  return addContent(MAGIC_SUBDIRMARKER, (uintptr_t)dir->getHeader());
}

bool SPFS::Directory::addContent(std::shared_ptr<SPFS::File> file){
  return addContent(MAGIC_FILEMARKER, (uintptr_t)file->getHeader());
}

std::shared_ptr<SPFS::Directory> SPFS::Directory::createDirectory(const std::string& name){
  auto new_dir = _fs->createDirectory(shared_from_this(), name);
  if(new_dir == nullptr){
    return nullptr;
  }

  addContent(new_dir);

  return new_dir;
}

std::shared_ptr<SPFS::File> SPFS::Directory::createFile(const std::string& name){
  auto new_file = _fs->createFile(shared_from_this(), name);
  if(new_file == nullptr){
    return nullptr;
  }

  addContent(new_file);

  return new_file;
}

std::shared_ptr<SPFS::Directory> SPFS::Directory::openSubdirectory(const std::string& name){
  auto subdirs = getSubdirectories();
  for(const auto& dir : subdirs){
    if(dir->getName() == name){
      return dir;
    }
  }
  return nullptr;
}

std::shared_ptr<SPFS::File> SPFS::Directory::openFile(const std::string& name){
  auto files = getFiles();
  for(const auto& file : files){
    if(file->getName() == name){
      return file;
    }
  }
  return nullptr;
}

bool SPFS::Directory::remove(std::shared_ptr<SPFS::File> file){
  return removeContent((uintptr_t)file->getHeader());
}
bool SPFS::Directory::remove(std::shared_ptr<SPFS::Directory> dir){
  return removeContent((uintptr_t)dir->getHeader());
}

std::shared_ptr<SPFS::Directory> SPFS::Directory::createHardlink(std::shared_ptr<SPFS::Directory> subdir){
  if(subdir == nullptr) {
    return nullptr;
  }
  if(!addContent(subdir)) {
    return nullptr;
  }
  return subdir;
}
std::shared_ptr<SPFS::File> SPFS::Directory::createHardlink(std::shared_ptr<SPFS::File> file, const std::string& new_name){
  if(file == nullptr) {
    return nullptr;
  }
  if(new_name != "") {
    // Create a new file with the new name that points to the same content
    auto hardlink_file = _fs->createFile(shared_from_this(), new_name, file->getContentHeader());
    if(hardlink_file == nullptr) {
      return nullptr;
    }
    if(!addContent(hardlink_file)) {
      return nullptr;
    }
    return hardlink_file;
  }
  if(!addContent(file)) {
    return nullptr;
  }
  return file;
}

