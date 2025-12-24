#pragma once

#include "../ICommand.h"
#include "Console.h"
#include <iostream>

class CatCommand : public ICommand {
public:
  // Constructor
  CatCommand(const Console &console) : _console(console) {}
  // Returns the name of the command
  const std::string getName() const override { return "cat"; }

  const std::string getHelp() const override {
    return "Usage: cat <filename>\n"
           "       Displays the contents of the specified file.";
  }

  // Executes the command
  int execute(const std::vector<std::string> &args) override {
    if(_console.currentDirectory == nullptr) {
      std::cout << "No filesystem loaded." << std::endl;
      return 1; // Return 1 to indicate error
    }
    if (args.size() < 2) {
      std::cout << getHelp() << std::endl;
      return 1; // Return 1 to indicate error
    }
    auto currentDir = _console.currentDirectory;
    std::string filename = args[1];
    auto file = currentDir->openFile(filename);
    if (!file) {
      std::cout << "File not found: " << filename << std::endl;
      return 1; // Return 1 to indicate error
    }
    std::cout << file->readAsString() << std::endl;
    return 0; // Return 0 to indicate success
  }

private:
  const Console &_console; // Pointer to the console object
};