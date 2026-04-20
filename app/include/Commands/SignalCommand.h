#pragma once

#include "ICommand.h"
#include "Utils/Signal.h"
#include "Utils/ValueConverter.h"
#include "Mainloop.h"
#include "ITask.h"
#include <string>

class SignalCommand : public ICommand {
private:
  class SignalTask : public ITask {
  public:
    SignalTask(const std::string &command) : _command(command) {}

    void setPID(TaskPID pid) {
        _pid = pid;
    }

    TaskPID getPID() const {
        return _pid;
    }
    
    bool ExecuteTask(TaskPID pid) override {
        std::cout << "Executing command: " << _command << std::endl;
        return false;
    }

    const std::string getName() const override {
        return _command;
    }

  private:
    std::string _command;
    TaskPID _pid;
  };

public:
  SignalCommand(Mainloop &mainloop) : _mainloop(mainloop) {}

  // Returns the name of the command
  const std::string getName() const override { return "signal"; }

  const std::string getHelp() const override {
    return "Usage: signal add <signal> <command>\n"
           "       signal list\n"
           "       signal stop <PID>\n"
           "       signal emit <signal>\n\n"
           "       list:  Lists all registered signal handlers\n"
           "       stop:  Stops a signal handler by its PID\n"
           "       emit:  Triggers a signal to test handlers\n"
           "       add:   Registers a new signal handler with the specified signal filter and command to execute\n"
           "         <signal>: A 4-character string or a hexadecimal number representing the signal\n"
           "         <command>: The command to execute when the signal is received";
  }

  int execute(const std::vector<std::string> &args) override {
    if (args.size() < 2) {
      std::cout << getHelp() << std::endl;
      return -1;
    }

    if (args[1] == "list") {
        std::cout << "Registered signal handlers:" << std::endl;
        std::cout << "PID  - Signal Filter  - Command" << std::endl;
        for (const auto &task : _signalTasks) {
            std::cout << task.getPID() << "  - " << SignalConverter::toString(_mainloop.getSignalFilter(task.getPID())) << "  - " << task.getName() << std::endl;
        }
        return 0;
    } else if (args[1] == "stop") {
        if (args.size() != 3) {
            std::cout << getHelp() << std::endl;
            return -1;
        }
        int pid = std::stoi(args[2]);
        for (int n = 0; n < _signalTasks.size(); n++) {
            const auto &task = _signalTasks[n];
            if (task.getPID() == pid) {
                _mainloop.killTask(task.getPID());
                _signalTasks.erase(_signalTasks.begin() + n);
                std::cout << "Stopped signal handler with PID: " << task.getPID() << std::endl;
                return 0;
            }
        }
        std::cout << "No signal handler found with PID: " << pid << std::endl;
        return -1;
    } else if (args[1] == "emit") {
        if (args.size() != 3) {
            std::cout << getHelp() << std::endl;
            return -1;
        }
        Signal signal = SignalConverter::fromString(args[2]).signal;
        _mainloop.triggerSignal(signal);
        std::cout << "Emitted signal: " << SignalConverter::toString(signal) << std::endl;
        return 0;
    } else if (args[1] != "add") {
      std::cout << getHelp() << std::endl;
      return -1;
    }

    if (args.size() < 4) {
      std::cout << getHelp() << std::endl;
      return -1;
    }
    SignalFilter filter = SignalConverter::fromString(args[2]);

    SignalTask task(args[3]);
    _signalTasks.push_back(task);

    TaskPID pid = _mainloop.registerSignalTask(&_signalTasks.back(), filter);
    _signalTasks.back().setPID(pid);

    std::cout << "Registered signal handler for signal: " << SignalConverter::toString(filter) << "(with mask: " <<  ValueConverter::toString(filter.mask, IntegerStringFormat::HEX) << ") PID: " << pid << std::endl;

    return 0;
  }

private:
  Mainloop &_mainloop;
  std::vector<SignalTask> _signalTasks;
};