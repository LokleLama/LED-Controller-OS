#include "SPFS.h"
#include "flash.h"
#include <cstring>

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

  int current = 0;
  while(contentHeaders[current].type != 0xFFFF && current < max_count) {
    current++;
  }

  if(current >= max_count) {
    //TODO: Implement directory extension
    return false; // No space for new content
  }

  contentHeaders[current].type = type; // Directory type
  contentHeaders[current].block_offset = (int16_t)((content_address - (uintptr_t)getHeader()) / FS_BLOCK_SIZE);

  if(Flash::write(buffer, getHeader()) < (int)buffer.size()) {
    return false;
  }
  return true;
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

