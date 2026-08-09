#pragma once

#include "../ICommand.h"
#include "../Utils/base64.h"
#include "../Utils/hexadecimal.h"
#include "Console.h"
#include <iostream>

class FixCommand : public ICommand {
public:
  // Constructor
  FixCommand(const Console &console) : _console(console) {}
  // Returns the name of the command
  const std::string getName() const override { return "fix"; }

  const std::string getHelp() const override {
    return "Usage: fix <file>\n"
           "\n"
           "       fix <file>\n"
           "           Fixes a file that has not been properly closed\n"
           "\n";
  }

  // Executes the command
  int execute(const std::vector<std::string> &args) override {
    if (args[1] == "-h" || args[1] == "--help" || args.size() < 2) {
        std::cout << getHelp() << std::endl;
        return 0;
    }

    if(args.size() < 2) {
        std::cout << "Error: Insufficient arguments provided." << std::endl;
        return -1;
    }

    std::string filename = args[1];
    auto file = _console.currentDirectory->openFile(filename);
    if (file == nullptr) {
        std::cout << "Error: Unable to open file '" << filename << "'" << std::endl;
        return -1;
    }

    if(file->fixUnfinishedContent()) {
      std::cout << "File '" << filename << "' has been fixed successfully." << std::endl;
      return 0;
    } else {
      std::cout << "Error: Unable to fix file '" << filename << "'. It may not have unfinished content or may be corrupted." << std::endl;
      return -1;
    }
  }

private:
  const Console &_console;

  std::shared_ptr<SPFS::File> _currentFile = nullptr;
};