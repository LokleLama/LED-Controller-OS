#pragma once

#include "../ICommand.h"
#include "hardware/uart.h"
#include <string>

class SetCommand : public ICommand {
public:
  // Returns the name of the command
  const std::string getName() const override { return "set"; }

  // Executes the command
  int execute(std::vector<std::string> args) override {
    if (args.size() < 3) {
      return showUsage();
    }

    if (args[1] == "baud") {
      int baudRate = std::stoi(args[2], nullptr, 10);
      setBaudRate(baudRate);
    } else if (args[1] == "format") {
      if (args[2].size() < 3) {
        return showUsage();
      }
      int bits = args[2][0] - '0';
      char parity = args[2][1];
      int stopBits = args[2][2] - '0';

      if (bits < 5 || bits > 8 ||
          (parity != 'n' && parity != 'o' && parity != 'e') ||
          (stopBits != 1 && stopBits != 2)) {
        return showUsage();
      }
      setFormat(bits, parity, stopBits);
    } else {
      return showUsage();
    }

    return 0;
  }

private:
  void setBaudRate(int baudRate) {
    int actual = uart_set_baudrate(uart0, baudRate);
    std::cout << "Setting baud rate to " << actual
              << " (requested: " << baudRate << ")" << std::endl;
  }

  void setFormat(int bits, char parity, int stopBits) {
    // Implement UART0 format change logic here
    // Example: UART0.setFormat(bits, parity, stopBits);
  }

  int showUsage() {
    std::cout << "Usage: set <baud|format> <value>" << std::endl;
    std::cout << "       Example: set baud 115200" << std::endl;
    std::cout << "       Example: set format 8n1" << std::endl;
    return -1; // Return -1 to indicate failure
  }
};
