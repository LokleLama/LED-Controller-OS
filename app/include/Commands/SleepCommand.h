#pragma once

#include "../ICommand.h"
#include "pico/stdlib.h"
#include <iostream>
#include <string>

class SleepCommand : public ICommand {
public:
  // Constructor
  SleepCommand() {}

  // Returns the name of the command
  const std::string getName() const override { return "sleep"; }

  const std::string getHelp() const override {
    return "Usage: sleep <milliseconds>\n"
           "\n"
           "Pauses execution for the specified number of milliseconds.\n"
           "Useful for timing in animation scripts.\n"
           "\n"
           "Examples:\n"
           "  sleep 100     - Pause for 100ms (0.1 seconds)\n"
           "  sleep 1000    - Pause for 1 second\n"
           "  sleep 50      - Pause for 50ms (good for smooth animations)\n"
           "\n"
           "Note: This is a blocking sleep. The console will not respond\n"
           "      to input during the sleep period.";
  }

  // Executes the command
  int execute(const std::vector<std::string> &args) override {
    if (args.size() < 2) {
      std::cout << "Usage: " << args[0] << " <milliseconds>" << std::endl;
      return -1;
    }

    // Use strtol for safer parsing without exceptions
    char* endptr;
    long milliseconds = strtol(args[1].c_str(), &endptr, 10);
    
    // Check if conversion was successful
    if (endptr == args[1].c_str() || *endptr != '\0') {
      std::cout << "Error: Invalid number format." << std::endl;
      return -1;
    }
    
    if (milliseconds < 0) {
      std::cout << "Error: Sleep time must be positive." << std::endl;
      return -1;
    }
    
    if (milliseconds > 60000) {
      std::cout << "Warning: Sleep time > 60 seconds, capping at 60s." << std::endl;
      milliseconds = 60000;
    }

    sleep_ms(static_cast<uint32_t>(milliseconds));
    return 0;
  }
};
