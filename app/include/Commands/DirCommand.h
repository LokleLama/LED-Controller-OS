#pragma once

#include "../ICommand.h"
#include "Console.h"
#include <iostream>

class DirCommand : public ICommand {
public:
  // Constructor
  DirCommand(const Console &console) : _console(console) {}
  // Returns the name of the command
  const std::string getName() const override { return "dir"; }

  // Executes the command
  int execute(const std::vector<std::string> &args) override {
    auto currentDir = _console.currentDirectory;
    auto files = currentDir->getFiles();
    size_t total_size = 0;
    size_t total_size_on_disk = currentDir->getSizeOnDisk();
    for (const auto &file : files) {
      total_size += file->getSize();
      total_size_on_disk += file->getSizeOnDisk();
    }
    std::cout << " Directory of " << currentDir->getFullPath() << std::endl;
    std::cout << " Total Files: " << files.size() << " Size: " << total_size << " bytes, Size on disk: " << total_size_on_disk << " bytes" << std::endl;
    auto subdirs = currentDir->getSubdirectories();
    for (const auto &dir : subdirs) {
      std::cout << " <DIR>  " << dir->getName() << " (" << dir->getDirectoryCount() + dir->getFileCount() << " entries)" << std::endl;
    }
    for (const auto &file : files) {
      std::cout << " - v" << file->getVersion() << " - " << file->getName() << " (" << file->getSize() << " bytes)" << std::endl;
    }
    return 0; // Return 0 to indicate success
  }

private:
  const Console &_console; // Pointer to the console object
};