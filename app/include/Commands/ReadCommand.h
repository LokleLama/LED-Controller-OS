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

  // Executes the command
  int execute(const std::vector<std::string> &args) override {
    if (args.size() < 3) {
      std::cout << "Usage: " << args[0] << " <address> <length>" << std::endl;
      return -1; // Return -1 to indicate an error
    }
    int addr = std::stoi(args[1], nullptr, 0);
    int len = std::stoi(args[2], nullptr, 0);
    std::vector<uint8_t> buffer(len);
    int result = Flash::read(buffer, addr);
    if (result == 0) {
      std::cout << "Data read from flash: 0x";
      std::cout << std::hex << std::setfill('0') << std::setw(8) << addr
                << std::dec << std::endl;

      for (int i = 0; i < len;) {
        for (int j = 0; j < 16 && i < len; ++j, i++) {
          std::cout << std::hex << std::setfill('0') << std::setw(2)
                    << static_cast<int>(buffer[i + j]) << " ";
        }
        std::cout << std::dec << std::endl;
      }
    } else {
      std::cout << "Failed to read from flash." << std::endl;
    }
    return 0; // Return 0 to indicate success
  }
};