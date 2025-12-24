#pragma once

#include "../ICommand.h"
#include "Console.h"
#include <iostream>

class MakeDirCommand : public ICommand {
public:
  // Constructor
  MakeDirCommand(Console &console) : _console(console) {}
  // Returns the name of the command
  const std::string getName() const override { return "md"; }

  const std::string getHelp() const override {
    return "Usage: md <directory>\n"
           "       Creates a new directory with the specified name and changes into it.";
  }

  // Executes the command
  int execute(const std::vector<std::string> &args) override {
    if(_console.currentDirectory == nullptr) {
      std::cout << "No filesystem loaded." << std::endl;
      return 1; // Return 1 to indicate error
    }
    if(args.size() < 2) {
      std::cout << getHelp() << std::endl;
      return -1; // Return 1 to indicate failure
    }
    auto currentDir = _console.currentDirectory;
    auto subdirs = currentDir->getSubdirectories();
    for(const auto& dir : subdirs) {
      if(dir->getName() == args[1]) {
        std::cout << "Directory already exists: " << args[1] << std::endl;
        _console.currentDirectory = dir;
        return -1; // Return 1 to indicate failure
      }
    }
    auto subdir = currentDir->createDirectory(args[1]);
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