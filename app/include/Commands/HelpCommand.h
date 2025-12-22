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

  const std::string getHelp() const override {
    return "Usage: help [<command>]\n"
           "       Lists all available commands.\n"
           "       If a command is specified, displays detailed help for that command.";
  }

  // Executes the command
  int execute(const std::vector<std::string> &args) override {
    if (args.size() > 1) {
      auto command = _console.findCommand(args[1]);
      if (command) {
        std::cout << command->getHelp() << std::endl;
        return 0; // Return 0 to indicate success
      } else {
        std::cout << "Command not found: " << args[1] << std::endl;
        std::cout << getHelp() << std::endl;
        return -1; // Return -1 to indicate failure
      }
    }
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