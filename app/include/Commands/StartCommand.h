#pragma once

#include "ICommand.h"
#include <iostream>

#include "Mainloop.h"
#include "RTC/PicoTime.h"
#include "VariableStore/VariableStore.h"

class TimeTask {
public:
  TimeTask(const std::string& variableName, std::shared_ptr<PicoTime> picoTime) : _timeVariable(variableName), _picoTime(picoTime) {
    std::tm timeInfo = _picoTime->getTimeInfo();
    _lastSetTime = timeInfo;
    setTimeToVariable();
    Mainloop::getInstance().registerTimedTask([this]() { return setterTask(); }, 1000);
  }

  void stop() {
    _taskRunning = false;
  }

private:
  std::string _timeVariable;
  std::shared_ptr<PicoTime> _picoTime;
  bool _taskRunning = true;
  std::tm _lastSetTime;

  bool setterTask(){
    // Here you would add logic to set the specified variable to the current time every minute.
    // For demonstration purposes, we'll just print the current time.
    std::tm timeInfo = _picoTime->getTimeInfo();
    if(timeInfo.tm_min == _lastSetTime.tm_min){
      return _taskRunning; // Only update once per minute
    }
    _lastSetTime = timeInfo;
    setTimeToVariable();
    return _taskRunning;
  }

  void setTimeToVariable(){
    std::string buffer(10, '\0');
    std::strftime(buffer.data(), buffer.size(), "%H:%M", &_lastSetTime);
    VariableStore::getInstance().setVariable(_timeVariable, buffer);
  }
};

class StartCommand : public ICommand {
public:
  StartCommand(std::shared_ptr<PicoTime> picoTime) : _timeTask(nullptr), _picoTime(picoTime) {}
  // Returns the name of the command
  const std::string getName() const override { return "start"; }

  const std::string getHelp() const override {
    return "Usage: start <task> [parameters]\n"
           "       Starts a new background task.\n"
           "       <task>: The name of the task to start.\n"
           "       [parameters]: Optional parameters to pass to the task.\n"
           "\n"
           "       Possible tasks include:\n"
           "       - time: sets the time to a specific variabel in the form HH:MM every minute";
  }

  // Executes the command
  int execute(const std::vector<std::string> &args) override {
    if (args.size() < 2) {
      std::cout << getHelp() << std::endl;
      return -1; // Return -1 to indicate error
    }
    
    if (args[1] == "time") {
      if (args.size() < 3) {
        std::cout << "Error: Missing variable name for time task.\n" << getHelp() << std::endl;
        return -1; // Return -1 to indicate error
      }
      if(_timeTask){
        std::cout << "Error: Time task is already running.\n" << getHelp() << std::endl;
        return -1; // Return -1 to indicate error
      }
      
      if(!VariableStore::getInstance().getVariable(args[2])){
        std::cout << "Error: Variable '" << args[2] << "' does not exist.\n" << getHelp() << std::endl;
        return -1; // Return -1 to indicate error
      }
      _timeTask = std::make_shared<TimeTask>(args[2], _picoTime);
      std::cout << "Started time task to update variable '" << args[2] << "' every minute." << std::endl;
    } else {
      std::cout << "Error: Unknown task '" << args[1] << "'.\n" << getHelp() << std::endl;
      return -1; // Return -1 to indicate error
    }
    return 0; // Return 0 to indicate success
  }

private:
  std::shared_ptr<TimeTask> _timeTask;
  std::shared_ptr<PicoTime> _picoTime;
};