#pragma once

#include "ICommand.h"
#include "RTC/PicoTime.h"
#include <ctime>
#include <iostream>
#include <memory>
#include <string>

class TimeCommand : public ICommand {
public:
  // Returns the name of the command
  const std::string getName() const override { return "time"; }

  // Constructor that takes a shared pointer to PicoTime
  TimeCommand(std::shared_ptr<PicoTime> picoTime)
      : picoTime(std::move(picoTime)) {}

  // Executes the command
  int execute(const std::vector<std::string> &args) override {
    if (args.size() > 1 && args[1] == "get" || args.size() == 1) {
      // Get the current time
      auto timeInfo = picoTime->getTimeInfo();
      char buffer[20];
      std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeInfo);
      std::cout << "Current time: " << buffer << std::endl;
      return 0;
    }
    return -1; // Command not recognized
  }

private:
  std::shared_ptr<PicoTime> picoTime;
};