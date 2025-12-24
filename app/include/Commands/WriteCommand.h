#pragma once

#include "../Flash/flash.h"
#include "../ICommand.h"
#include "../Utils/base64.h"
#include <iomanip>
#include <iostream>

class WriteCommand : public ICommand {
public:
  // Returns the name of the command
  const std::string getName() const override { return "write"; }

  const std::string getHelp() const override {
    return "Usage: write <offset> [<bytes to write>]\n"
           "       Writes data to flash memory starting at <offset> for <length> bytes.\n"
           "           <offset> is the byte offset from the start of flash memory\n"
           "           <bytes to write> is the is the data array to write";
  }

  // Executes the command
  int execute(const std::vector<std::string> &args) override {
    if (args.size() < 3) {
      std::cout << getHelp() << std::endl;
      return -1; // Return -1 to indicate an error
    }
    int addr = std::stoi(args[1], nullptr, 0);

    std::vector<uint8_t> buffer(256);
    int result = Flash::read(buffer, addr);
    if (result == buffer.size()) {
      for(size_t i = 2; i < args.size(); ++i) {
        buffer[i - 2] = static_cast<uint8_t>(std::stoi(args[i], nullptr, 0));
      }

      result = Flash::write(buffer, addr);      
      if (result == buffer.size()) {
        std::cout << "Data written to flash... " << std::endl;
      } else {
        std::cout << "Failed to write to flash. (address = 0x" << std::hex << addr << std::dec << ", size = " << buffer.size() << ")" << std::endl;
      }
    } else {
      std::cout << "Failed to read from flash. (address = 0x" << std::hex << addr << std::dec << ", size = " << buffer.size() << ")" << std::endl;
    }
    return 0; // Return 0 to indicate success
  }
};