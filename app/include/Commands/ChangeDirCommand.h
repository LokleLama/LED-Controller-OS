#pragma once

#include "../ICommand.h"
#include "Console.h"
#include <iostream>

class ChangeDirCommand : public ICommand {
public:
  // Constructor
  ChangeDirCommand(Console &console) : _console(console) {}
  // Returns the name of the command
  const std::string getName() const override { return "cd"; }

  // Executes the command
  int execute(const std::vector<std::string> &args) override {
    if(_console.currentDirectory == nullptr) {
      std::cout << "No filesystem loaded." << std::endl;
      return 1; // Return 1 to indicate error
    }
    if(args.size() < 2) {
      std::cout << "Usage: cd <directory>" << std::endl;
      return -1; // Return 1 to indicate failure
    }
    if(args[1] == "..") {
      auto parentDir = _console.currentDirectory->getParent();
      if(parentDir != nullptr) {
        _console.currentDirectory = parentDir;
      }
      return 0;
    }
    auto currentDir = _console.currentDirectory;
    auto subdir = currentDir->openSubdirectory(args[1]);
    if(subdir == nullptr) {
      std::cout << "Directory not found: " << args[1] << std::endl;
      return -1; // Return 1 to indicate failure
    }
    _console.currentDirectory = subdir;
    return 0;
  }

private:
  Console &_console; // Reference to the console object
};