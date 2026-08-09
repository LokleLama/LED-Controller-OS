#pragma once

#include "../ICommand.h"
#include "Console.h"
#include <iostream>

class FSInfoCommand : public ICommand {
public:
  // Constructor
  FSInfoCommand(const Console &console) : _console(console) {}
  // Returns the name of the command
  const std::string getName() const override { return "fsinfo"; }

  const std::string getHelp() const override {
    return "Usage: fsinfo [--no-map]\n"
           "       Displays information about the file system.";
  }

  // Executes the command
  int execute(const std::vector<std::string> &args) override {
    auto fs = _console.getFileSystem();
    if(fs == nullptr) {
      std::cout << "No filesystem loaded." << std::endl;
      return 1; // Return 1 to indicate error
    }
    std::cout << " File System Information: " << fs->getFileSystemName() << " (version " << fs->getFileSystemVersion() << ")" << std::endl;
    std::cout << "  - Total Size: " << fs->getFileSystemSize() / 1024 << " kB" << std::endl;
    std::cout << "  - Block Size: " << fs->getBlockSize() << " bytes" << std::endl;
    auto usage_map =  fs->getBlockUsageMap();
    size_t free_blocks = 0;
    size_t used_blocks = 0;
    size_t used_file_blocks = 0;
    size_t reserved_file_blocks = 0;
    size_t used_dir_blocks = 0;
    size_t used_tag_blocks = 0;
    size_t bad_blocks = 0;
    for(const auto& state : usage_map) {
      switch(state) {
        case SPFS::BlockState::FREE:
          free_blocks++;
          break;
        case SPFS::BlockState::USED:
          used_blocks++;
          break;
        case SPFS::BlockState::USED_FILE:
          used_file_blocks++;
          break;
        case SPFS::BlockState::RESERVED_FILE:
          reserved_file_blocks++;
          break;
        case SPFS::BlockState::USED_DIR:
          used_dir_blocks++;
          break;
        case SPFS::BlockState::USED_TAG:
          used_tag_blocks++;
          break;
        case SPFS::BlockState::BAD:
          bad_blocks++;
          break;
      }
    }
    std::cout << "  - Free Space: " << free_blocks * fs->getBlockSize() / 1024 << " kB" << std::endl;
    std::cout << "  - Block Usage:" << std::endl;
    std::cout << "     * Free Blocks: " << free_blocks << std::endl;
    std::cout << "     * Used Blocks: " << used_blocks + used_file_blocks + used_dir_blocks + reserved_file_blocks + used_tag_blocks << std::endl;
    std::cout << "        - Used Blocks for Files      : " << used_file_blocks << std::endl;
    std::cout << "        - Reserved Blocks for Files  : " << reserved_file_blocks << std::endl;
    std::cout << "        - Used Blocks for Directories: " << used_dir_blocks << std::endl;
    std::cout << "        - Used Blocks for Tags       : " << used_tag_blocks << std::endl;
    std::cout << "     * Bad Blocks: " << bad_blocks << std::endl;

    if(args.size() <= 1 || args[1] != "--no-map") {
      std::cout << "Block Usage Map (.=Free, U=Used, F=Used by File, R=Reserved for File, D=Used by Directory, T=Used by Tag, B=Bad):" << std::endl;
      for(size_t i = 0; i < usage_map.size(); i++) {
        switch(usage_map[i]) {
          case SPFS::BlockState::FREE:
            std::cout << ".";
            break;
          case SPFS::BlockState::USED:
            std::cout << "U";
            break;
          case SPFS::BlockState::USED_FILE:
            std::cout << "F";
            break;
          case SPFS::BlockState::RESERVED_FILE:
            std::cout << "R";
            break;
          case SPFS::BlockState::USED_DIR:
            std::cout << "D";
            break;
          case SPFS::BlockState::USED_TAG:
            std::cout << "T";
            break;
          case SPFS::BlockState::BAD:
            std::cout << "B";
            break;
        }
        if((i + 1) % 16 == 0) {
          std::cout << " ";
        }
        if((i + 1) % 64 == 0) {
          std::cout << std::endl;
        }
      }
      std::cout << std::endl;
    }

    return 0; // Return 0 to indicate success
  }

private:
  const Console &_console; // Pointer to the console object
};