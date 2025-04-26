#pragma once

#include "../ICommand.h"
#include "Console.h"
#include <iostream>

class HelpCommand : public ICommand {
public:
  // Constructor
  HelpCommand(const Console &console) : _console(console) {}
  // Returns the name of the command
  const std::string getName() const override { return "help"; }

  // Executes the command
  int execute(const std::vector<std::string> &args) override {

    auto commandList = _console.getCommandList();
    std::cout << "Available commands:" << std::endl;
    for (const auto &command : commandList) {
      std::cout << " - " << command << std::endl;
    }
    return 0; // Return 0 to indicate success
  }

private:
  const Console &_console; // Pointer to the console object
};