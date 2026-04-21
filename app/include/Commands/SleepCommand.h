#pragma once

#include "ICommand.h"
#include "IVariableStore.h"
#include <string>

class SleepCommand : public ICommand {
public:
  SleepCommand(Mainloop &mainloop, Console &console) : mainloop(mainloop), console(console) {}

  // Returns the name of the command
  const std::string getName() const override { return "sleep"; }

  const std::string getHelp() const override {
    return "Usage: sleep <duration> [<pid>]\n"
           "       <duration>: Time to sleep in milliseconds\n"
           "       <pid>: Optional PID of the task to put to sleep";
  }

  int execute(const std::vector<std::string> &args) override {
    if (args.size() < 2) {
      std::cout << getHelp() << std::endl;
      return -1;
    }
    int duration = std::stoi(args[1]);

    if (args.size() >= 3) {
      int pid = std::stoi(args[2]);
      if(pid == 0){
        std::cout << "Cannot sleep task with PID 0" << std::endl;
        return -1;
      }
      mainloop.sleepTask(pid, duration);
    } else {
      mainloop.sleepTask(console.getPID(), duration);
    }
    return 0;
  }

private:
  Mainloop &mainloop;
  Console &console;
};
