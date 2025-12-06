#include "SPFS.h"
#include "flash.h"
#include <cstring>

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
  auto max_count = (int)((FS_BLOCK_SIZE - (dirheader->name_size_content_offset >> 8)) / sizeof(DirectoryContentHeader));

  int current = 0;
  while(contentHeaders[current].type != 0xFFFF && current < max_count) {
    current++;
  }

  if(current >= max_count) {
    //TODO: Implement directory extension
    return nullptr; // No space for new content
  }

  contentHeaders[current].type = MAGIC_SUBDIRMARKER; // Directory type
  contentHeaders[current].block_offset = (int16_t)(((uintptr_t)new_dir->getHeader() - (uintptr_t)getHeader()) / FS_BLOCK_SIZE);

  if(Flash::write(buffer, getHeader()) < (int)buffer.size()) {
    return nullptr;
  }

  return new_dir;
}

std::vector<std::shared_ptr<SPFS::Directory>> SPFS::Directory::getSubdirectories() {
  std::vector<std::shared_ptr<SPFS::Directory>> subdirs;

  auto contentHeaders = reinterpret_cast<const DirectoryContentHeader *>(reinterpret_cast<const uint8_t*>(getHeader()) + (getHeader()->name_size_content_offset >> 8));
  auto max_count = (int)((FS_BLOCK_SIZE - (getHeader()->name_size_content_offset >> 8)) / sizeof(DirectoryContentHeader));

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

  auto contentHeaders = reinterpret_cast<const DirectoryContentHeader *>(reinterpret_cast<const uint8_t*>(getHeader()) + (getHeader()->name_size_content_offset >> 8));
  auto max_count = (int)((FS_BLOCK_SIZE - (getHeader()->name_size_content_offset >> 8)) / sizeof(DirectoryContentHeader));

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
  return std::string(name_ptr, _header->name_size_content_offset & 0x00FF);
}

std::shared_ptr<SPFS::File> SPFS::Directory::createFile(const std::string& name){
  return _fs->createFile(shared_from_this(), name);
}

