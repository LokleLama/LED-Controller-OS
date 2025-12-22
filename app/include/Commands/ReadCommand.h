#pragma once

#include "../Flash/flash.h"
#include "../ICommand.h"
#include "../Utils/base64.h"
#include <iomanip>
#include <iostream>

class ReadCommand : public ICommand {
public:
  // Returns the name of the command
  const std::string getName() const override { return "read"; }

  const std::string getHelp() const override {
    return "Usage: read <offset> <length>\n"
           "       Reads data from flash memory starting at <offset> for <length> bytes.\n"
           "           <offset> is the byte offset from the start of flash memory\n"
           "           <length> is the number of bytes to read";
  }

  // Executes the command
  int execute(const std::vector<std::string> &args) override {
    if (args.size() < 3) {
      std::cout << getHelp() << std::endl;
      return -1; // Return -1 to indicate an error
    }
    int addr = std::stoi(args[1], nullptr, 0);
    int len = std::stoi(args[2], nullptr, 0);
    std::vector<uint8_t> buffer(len);
    int result = Flash::read(buffer, addr);
    if (result == len) {
      std::cout << "Data read from flash: 0x";
      std::cout << std::hex << std::setfill('0') << std::setw(8) << Flash::readPointer(addr);

      std::cout << std::endl;
      for (int i = 0; i < len;) {
        for (int j = 0; j < 16 && i < len; ++j, i++) {
          std::cout << std::setw(2) << static_cast<int>(buffer[i]) << " ";
        }
        std::cout << std::endl;
      }
      std::cout << std::dec;
    } else {
      std::cout << "Failed to read from flash. (address = 0x" << std::hex << addr << std::dec << ", size = " << len << ")" << std::endl;
    }
    return 0; // Return 0 to indicate success
  }
};