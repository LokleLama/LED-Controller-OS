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
    : _device(device), _mainloop(mainloop) {}

  bool ExecuteTask(TaskPID pid) override {
    if (isFinished()) {
      _device->setLevel(0);
      _mainloop.killTask(pid); // Remove the task from the mainloop when finished
      return false; // No more beeps to play, task is finished
    }

    if(_data_ptr) {
      const uint16_t frequency = _data_ptr[_current_index * 2];
      const uint16_t duration = _data_ptr[_current_index * 2 + 1];
      _current_index++;
      return setBeep(pid, frequency, duration);
    }

    const auto& [frequency, duration] = _beep_sequence[_current_index];
    _current_index++;

    return setBeep(pid, frequency, duration);
  }

  const std::string getName() const override {
    return "BeepCommandTask - " + _device->getName();
  }

  void addBeep(uint16_t frequency, uint16_t duration) {
    _beep_sequence.emplace_back(frequency, duration);
  }

  void setBeepData(const uint16_t* data, size_t count) {
    _data_ptr = data;
    _data_count = count;
  }

  bool isFinished() const {
    if(_data_ptr) {
      return _current_index >= _data_count;
    }
    return _current_index >= _beep_sequence.size();
  }

private:
  std::shared_ptr<PWMDevice> _device;
  Mainloop& _mainloop;

  std::vector<std::pair<uint16_t, uint16_t>> _beep_sequence;
  size_t _current_index = 0;

  const uint16_t* _data_ptr = nullptr;
  size_t _data_count = 0;

  bool setBeep(TaskPID pid, uint16_t frequency, uint16_t duration) {
    if (frequency <= 0) {
      _device->setLevel(0);
    } else {
      _device->configurePWM(frequency);
    }

    if(duration > 0) {
      _mainloop.sleepTask(pid, duration);
    }
    return true;
  }
};

class BeepCommand : public ICommand {
public:
  // Constructor
  BeepCommand(Mainloop &mainloop, const Console &console, DeviceRepository &deviceRepo) : 
      _mainloop(mainloop), 
      _console(console),
      _deviceRepo(deviceRepo) {}

  // Returns the name of the command
  const std::string getName() const override { return "beep"; }

  const std::string getHelp() const override {
    return "Usage: beep <deviceName> <frequency> <duration> [<frequency> <duration> ...]\n"
           "       beep <deviceName> play <fileName>\n"
           "       Generates a beep sound with the specified <frequency> (Hz) and <duration> (ms).\n"
           "       Multiple frequency-duration pairs can be provided to create a sequence of beeps.\n"
           "           <deviceName> is the name of the PWM device to use for the beep\n"
           "           <fileName> is the name of the file containing the beep sequence\n"
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

    if(args[2] == "play") {
      return playBeepSequenceFromFile(args[3], device);
    }

    auto task = std::make_shared<BeepCommandTask>(device, _mainloop);
    
    for (size_t i = 2; i + 1 < args.size(); i += 2) {
      int frequency = std::strtol(args[i].c_str(), nullptr, 10);
      int duration = std::strtol(args[i + 1].c_str(), nullptr, 10);

      if(frequency < 0 || duration < 0 || frequency > 65000 || duration > 65000) {
        std::cout << "Invalid frequency or duration: " << args[i] << " " << args[i + 1] << std::endl;
        return -1; // Return -1 to indicate failure
      }

      task->addBeep(static_cast<uint16_t>(frequency), static_cast<uint16_t>(duration));
    }

    _activeTasks.push_back(task);
    _mainloop.registerRegularTask(task.get());

    return 0; // Return 0 to indicate success
  }
private:
  Mainloop &_mainloop;
  const Console &_console;
  DeviceRepository &_deviceRepo;

  std::vector<std::shared_ptr<BeepCommandTask>> _activeTasks;

  int playBeepSequenceFromFile(const std::string& fileName, std::shared_ptr<PWMDevice> device) {
    std::unique_ptr<dataFileReader> reader = std::make_unique<dataFileReader>(_console.currentDirectory, fileName);
    if (!reader->isExpectedFile("BEEP")) {
      std::cout << "Invalid file header: " << fileName << std::endl;
      return -1;
    }
    
    size_t data_size = 0;
    const uint16_t* data = reinterpret_cast<const uint16_t*>(reader->getFieldData(0xA470, &data_size));

    // make sure data is aligned to 2 bytes
    if (!data || reinterpret_cast<uintptr_t>(data) % alignof(uint16_t) != 0) {
      std::cout << "Data is not properly aligned in file: " << fileName << std::endl;
      return -1;
    }

    if (data_size == 0 || data_size % (2 * sizeof(uint16_t)) != 0) {
      std::cout << "Data size is invalid in file: " << fileName << std::endl;
      return -1;
    }

    auto task = std::make_shared<BeepCommandTask>(device, _mainloop);
    task->setBeepData(data, data_size / (2 * sizeof(uint16_t)));
    _activeTasks.push_back(task);
    _mainloop.registerRegularTask(task.get());

    return 0;
  }
};