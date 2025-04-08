#pragma once

#include "../ICommand.h"
#include <iostream>

class EchoCommand : public ICommand {
public:
  // Returns the name of the command
  const std::string getName() const override { return "echo"; }

  // Executes the command
  int execute(const std::vector<std::string> &args) override {
    for (size_t i = 1; i < args.size(); ++i) {
      std::cout << args[i] << " ";
    }
    std::cout << std::endl;
    return 0; // Return 0 to indicate success
  }
};