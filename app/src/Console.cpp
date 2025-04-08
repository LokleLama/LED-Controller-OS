#include "Console.h"
#include "hardware/clocks.h"
#include "pico/stdlib.h"
#include "tusb.h"
#include <cstddef>
#include <iostream>
#include <sstream>

bool Console::registerCommand(std::shared_ptr<ICommand> command) {
  if (!command) {
    return false;
  }
  commandList.push_back(command);
  return true;
}

std::shared_ptr<ICommand> Console::findCommand(const std::string &name) const {
  for (const auto &cmd : commandList) {
    if (cmd->getName() == name) {
      return cmd;
    }
  }
  return nullptr;
}

void Console::consoleTask() {
  if (!isConnected && tud_cdc_n_connected(INTERFACE_NUMBER)) {
    isConnected = true;
    OnNewConnection();
  }

  if (tud_cdc_n_available(INTERFACE_NUMBER)) {
    char buf[64];

    uint32_t count = tud_cdc_n_read(INTERFACE_NUMBER, buf, sizeof(buf));
    tud_cdc_n_write(INTERFACE_NUMBER, buf, count);

    for (uint32_t i = 0; i < count; i++) {
      if (buf[i] == '\r' || buf[i] == '\n') {
        // Process the command
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream stream(inputBuffer);
        while (stream >> token) {
          tokens.push_back(token);
        }

        std::string command = tokens.empty() ? "" : tokens[0];
        inputBuffer.clear();

        auto cmd = findCommand(command);

        std::cout << std::endl;
        if (cmd != nullptr) {
          cmd->execute(tokens);
        } else {
          std::cout << "Unknown command: " << command << std::endl;
        }
        outputPrompt();
      } else if (buf[i] == 0x7F) { // Backspace
        if (!inputBuffer.empty()) {
          inputBuffer.pop_back();
        }
      } else {
        inputBuffer += buf[i];
      }
    }
    tud_cdc_n_write_flush(INTERFACE_NUMBER);
  }
}

void Console::OnNewConnection() const {
  std::cout << "Clock Rate: " << clock_get_hz(clk_sys) << std::endl;
  std::cout << "Console connected. Type 'exit' to quit." << std::endl;
  outputPrompt();
}

void Console::outputPrompt() const {
  std::cout << "> ";
  std::cout.flush();
}