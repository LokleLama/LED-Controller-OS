#pragma once

#include "ICommand.h"
#include "Utils/Signal.h"
#include "Utils/ValueConverter.h"
#include "Mainloop.h"
#include <string>

class SignalCommand : public ICommand {
public:
  SignalCommand(Mainloop &mainloop) : _mainloop(mainloop) {}

  // Returns the name of the command
  const std::string getName() const override { return "signal"; }

  const std::string getHelp() const override {
    return "Usage: signal <signal> <command>\n"
           "       <signal>: A 4-character string or a hexadecimal number representing the signal\n"
           "       <command>: The command to execute when the signal is received";
  }

  int execute(const std::vector<std::string> &args) override {
    if (args.size() < 3) {
      std::cout << getHelp() << std::endl;
      return -1;
    }
    SignalFilter filter = SignalConverter::fromString(args[1]);

    std::cout << "Registered signal handler for signal: " << SignalConverter::toString(filter) << "(with mask: " <<  ValueConverter::toString(filter.mask, IntegerStringFormat::HEX) << ")" << std::endl;

    //std::string command = args[2];
    //Mainloop::getInstance().addSignalHandler(filter, command);

    return 0;
  }

private:
  Mainloop &_mainloop;
};