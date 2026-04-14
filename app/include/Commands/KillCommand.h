#pragma once

#include "ICommand.h"
#include "Mainloop.h"
#include "Utils/ValueConverter.h"
#include <string>

class KillCommand : public ICommand {
public:
  KillCommand(Mainloop &mainloop) : _mainloop(mainloop) {}

  // Returns the name of the command
  const std::string getName() const override { return "kill"; }

  const std::string getHelp() const override {
    return "Usage: kill <task_id>\n"
           "       Kills the specified task.";
  }

  int execute(const std::vector<std::string> &args) override {
    if (args.size() < 2) {
      std::cout << getHelp() << std::endl;
      return -1;
    }
    int taskId = ValueConverter::toInt(args[1]);
    _mainloop.killTask(taskId);
    std::cout << "Task " << taskId << " has been killed." << std::endl;
    return 0;
  }

private:
  Mainloop &_mainloop;
};
