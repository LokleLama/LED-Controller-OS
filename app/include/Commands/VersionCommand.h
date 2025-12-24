#pragma once

#include "ICommand.h"
#include <iostream>

class VersionCommand : public ICommand {
public:
  // Returns the name of the command
  const std::string getName() const override { return "version"; }

  const std::string getHelp() const override {
    return "Usage: version\n"
           "       Displays the software version.";
  }

  // Executes the command
  int execute(const std::vector<std::string> &args) override {
    std::cout << "Software Version: 1.0.0" << std::endl;
    return 0; // Return 0 to indicate success
  }
};