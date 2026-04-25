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
    return "Usage: task [pause <task_id> <milliseconds>]\n"
           "       Shows the currently running tasks.\n"
           "       pause <task_id> <milliseconds>: Pauses the task with the given ID for the specified duration.";
  }

  // Executes the command
  int execute(const std::vector<std::string> &args) override {
    if(args.size() == 4 && args[1] == "pause") {
      TaskPID pid = std::strtol(args[2].c_str(), nullptr, 0);
      uint32_t duration = std::strtoul(args[3].c_str(), nullptr, 0);
      if(Mainloop::getInstance().sleepTask(pid, duration)) {
        std::cout << "Task with ID " << pid << " has been paused for " << duration << " milliseconds." << std::endl;
        return 0;
      } else {
        std::cout << "Failed to pause task with ID " << pid << std::endl;
        return -1;
      }
    }

    std::cout << "Currently registered tasks:" << std::endl;
    Mainloop::getInstance().OuptutTaskInformation();
    return 0; // Return 0 to indicate success
  }

};