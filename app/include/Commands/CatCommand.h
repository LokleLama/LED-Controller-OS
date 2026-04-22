#pragma once

#include "../ICommand.h"
#include "Console.h"
#include <iostream>
#include <sstream>
#include <iomanip>

class CatCommand : public ICommand {
public:
  // Constructor
  CatCommand(const Console &console) : _console(console) {}
  // Returns the name of the command
  const std::string getName() const override { return "cat"; }

  const std::string getHelp() const override {
    return "Usage: cat [-${version_offset}] [--hex] <filename>\n"
           "       Displays the contents of the specified file.\n"
           "       Options:\n"
           "         -${version_offset}  Display the file in the version ${current - offset}\n"
           "         --hex               Display the file contents in hexadecimal format";
  }

  // Executes the command
  int execute(const std::vector<std::string> &args) override {
    if(_console.currentDirectory == nullptr) {
      std::cout << "No filesystem loaded." << std::endl;
      return -1; // Return -1 to indicate error
    }
    if (args.size() < 2) {
      std::cout << getHelp() << std::endl;
      return -1; // Return -1 to indicate error
    }

    std::string filename = args[1];
    auto currentDir = _console.currentDirectory;
    bool hexOutput = false;
    int version = -1;

    for (size_t i = 1; i < args.size(); i++) {
      if (args[i] == "--hex") {
        hexOutput = true;
      } else if (args[i][0] == '-' && isdigit(args[i][1])) {
        version = std::strtol(args[i].substr(1).c_str(), nullptr, 0);
        if (version < 0) {
          std::cout << "Invalid version number: " << args[i].substr(1) << std::endl;
          return -1; // Return -1 to indicate error
        }
      } else {
        filename = args[i];
      }
    }
    
    auto file = currentDir->openFile(filename);
    if (!file) {
      std::cout << "File not found: " << filename << std::endl;
      return -1; // Return -1 to indicate error
    }

    auto open_version = file->getVersion();
    if(version >= 0) {
      if(version >= open_version) {
        std::cout << "Requested version " << version << " is greater then or equal to the current version " << open_version << std::endl;
        return -1; // Return -1 to indicate error
      }
      open_version -= version;
    }
    
    auto filereader = file->openVersion(open_version);
    if(!filereader) {
      std::cout << "Failed to open file version " << open_version << std::endl;
      return -1; // Return -1 to indicate error
    }

    if (hexOutput) {
      auto content = filereader->readBytes();
      for (size_t i = 0; i < content.size(); i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(content[i]) << " ";
        if ((i + 1) % 16 == 0) {
          std::cout << std::endl;
        }
      }
      std::cout << std::dec << std::endl; // Reset to decimal
      return 0; // Return 0 to indicate success
    }
    
    std::cout << filereader->readAsString() << std::endl;

    return 0; // Return 0 to indicate success
  }

private:
  const Console &_console; // Pointer to the console object
};