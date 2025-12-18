#pragma once

#include "../ICommand.h"
#include "Console.h"

class ExitCommand : public ICommand {
public:
    // Constructor
    ExitCommand(Console &console) : _console(console) {}

  // Returns the name of the command
  const std::string getName() const override { return "exit"; }

  // Executes the command
  int execute(const std::vector<std::string> &args) override {
    _console.Stop();
    return 0;
  }

private:
    Console &_console;
};