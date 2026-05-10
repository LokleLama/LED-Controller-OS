#pragma once

#include "ICommand.h"
#include "Mainloop.h"
#include "ITask.h"
#include "deviceController/DeviceRepository.h"
#include "devices/PWMDevice.h"
#include <iomanip>
#include <iostream>

class BeepCommandTask : public ITask {
public:
  BeepCommandTask(std::shared_ptr<PWMDevice> device, Mainloop& mainloop)
    : _device(device), _mainloop(mainloop), _current_index(0) {}

  bool ExecuteTask(TaskPID pid) override {
    if (isFinished()) {
      _device->setLevel(0);
      _mainloop.killTask(pid); // Remove the task from the mainloop when finished
      return false; // No more beeps to play, task is finished
    }

    const auto& [frequency, duration] = _beep_sequence[_current_index];
    _current_index++;
    if (frequency <= 0) {
      _device->setLevel(0);
    } else {
      _device->configurePWM(frequency);
    }

    if(duration <= 0) {
      return true;
    }
    _mainloop.sleepTask(pid, duration);
    return true; // Task is still active
  }

  const std::string getName() const override {
    return "BeepCommandTask - " + _device->getName();
  }

  void addBeep(int frequency, int duration) {
    _beep_sequence.emplace_back(frequency, duration);
  }

  bool isFinished() const {
    return _current_index >= _beep_sequence.size();
  }

private:
  std::shared_ptr<PWMDevice> _device;
  Mainloop& _mainloop;
  std::vector<std::pair<int, int>> _beep_sequence;
  size_t _current_index;
};

class BeepCommand : public ICommand {
public:
  // Constructor
  BeepCommand(Mainloop &mainloop, DeviceRepository &deviceRepo) : 
      _mainloop(mainloop), 
      _deviceRepo(deviceRepo) {}

  // Returns the name of the command
  const std::string getName() const override { return "beep"; }

  const std::string getHelp() const override {
    return "Usage: beep <deviceName> <frequency> <duration> [<frequency> <duration> ...]\n"
           "       Generates a beep sound with the specified <frequency> (Hz) and <duration> (ms).\n"
           "       Multiple frequency-duration pairs can be provided to create a sequence of beeps.\n"
           "           <deviceName> is the name of the PWM device to use for the beep\n"
           "           <frequency> is the frequency of the beep in Hertz\n"
           "           <duration> is the duration of the beep in milliseconds";
  }

  // Executes the command
  int execute(const std::vector<std::string> &args) override {
    // remove finished tasks from the active tasks list
    _activeTasks.erase(std::remove_if(_activeTasks.begin(), _activeTasks.end(), [](const std::shared_ptr<BeepCommandTask>& t) { return t->isFinished(); }), _activeTasks.end());

    if (args.size() < 4) {
      std::cout << getHelp() << std::endl;
      return -1; // Return -1 to indicate an error
    }

    auto device = _deviceRepo.getDevice<PWMDevice>("PWM", args[1]);
    if (!device) {
      std::cout << "Device not found: " << args[1] << std::endl;
      return -1; // Return -1 to indicate failure
    }

    auto task = std::make_shared<BeepCommandTask>(device, _mainloop);
    _activeTasks.push_back(task);
    
    for (size_t i = 2; i + 1 < args.size(); i += 2) {
      int frequency = std::strtol(args[i].c_str(), nullptr, 10);
      int duration = std::strtol(args[i + 1].c_str(), nullptr, 10);

      task->addBeep(frequency, duration);
    }

    _mainloop.registerRegularTask(task.get());

    return 0; // Return 0 to indicate success
  }
private:
  Mainloop &_mainloop;
  DeviceRepository &_deviceRepo;

  std::vector<std::shared_ptr<BeepCommandTask>> _activeTasks;
};