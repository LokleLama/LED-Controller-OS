#pragma once

#include "../ICommand.h"
#include "Mainloop.h"
#include <iostream>

class TaskCommand : public ICommand {
public:
  // Constructor
  TaskCommand() {}
  // Returns the name of the command
  const std::string getName() const override { return "task"; }

  const std::string getHelp() const override {
    return "Usage: task\n"
           "       Shows the currently running tasks.";
  }

  // Executes the command
  int execute(const std::vector<std::string> &args) override {
    std::cout << "Currently registered tasks:" << std::endl;
    Mainloop::getInstance().OuptutTaskInformation();
    return 0; // Return 0 to indicate success
  }

};