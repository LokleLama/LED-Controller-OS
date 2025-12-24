#pragma once

#include "../ICommand.h"
#include "Console.h"
#include <iostream>

#include "../Flash/SPFS.h"

class MakeFilesystemCommand : public ICommand {
public:
  // Constructor
  MakeFilesystemCommand(Console &console) : _console(console) {}
  // Returns the name of the command
  const std::string getName() const override { return "mkfs"; }

  const std::string getHelp() const override {
    return "Usage: mkfs\n"
           "       Creates a new filesystem within the flash memory.\n"
           "       WARNING: This will ERASE any existing data in the filesystem area.";
  }

  // Executes the command
  int execute(const std::vector<std::string> &args) override {
    std::cout << "Creating filesystem at " << SPFS_FLASH_OFFSET << " with size " << SPFS_FLASH_SIZE << std::endl;
    std::cout.flush();

    std::shared_ptr<SPFS> fs = std::make_shared<SPFS>();
    std::shared_ptr<SPFS::Directory> rootDir = fs->createNewFileSystem(SPFS_FLASH_OFFSET, SPFS_FLASH_SIZE, "LEDControllerFS", "root");
    if (!rootDir) {
      std::cout << "No filesystem found. Creation failed!" << std::endl;
      return 1; // Return 1 to indicate error
    }

    _console.setFileSystem(fs);
    std::cout << "Filesystem loaded into console." << std::endl;
    return 0; // Return 0 to indicate success
  }

private:
  Console &_console; // Pointer to the console object
};