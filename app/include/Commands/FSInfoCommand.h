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

  // Executes the command
  int execute(const std::vector<std::string> &args) override {
    auto fs = _console.getFileSystem();
    if(fs == nullptr) {
      std::cout << "No filesystem loaded." << std::endl;
      return 1; // Return 1 to indicate error
    }
    std::cout << " File System Information:" << std::endl;
    std::cout << "  - Total Size: " << fs->getFileSystemSize() << " bytes" << std::endl;
    auto usage_map =  fs->getBlockUsageMap();
    size_t free_blocks = 0;
    size_t used_blocks = 0;
    size_t used_file_blocks = 0;
    size_t used_dir_blocks = 0;
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
        case SPFS::BlockState::USED_DIR:
          used_dir_blocks++;
          break;
        case SPFS::BlockState::BAD:
          bad_blocks++;
          break;
      }
    }
    std::cout << "  - Block Usage:" << std::endl;
    std::cout << "     * Free Blocks: " << free_blocks << std::endl;
    std::cout << "     * Used Blocks: " << used_blocks + used_file_blocks + used_dir_blocks << std::endl;
    std::cout << "        - Used Blocks for Files      : " << used_file_blocks << std::endl;
    std::cout << "        - Used Blocks for Directories: " << used_dir_blocks << std::endl;
    std::cout << "     * Bad Blocks: " << bad_blocks << std::endl;
    return 0; // Return 0 to indicate success
  }

private:
  const Console &_console; // Pointer to the console object
};