#pragma once

#include "ICommand.h"
#include "Mainloop.h"
#include "ITask.h"
#include "Console.h"
#include "deviceController/DeviceRepository.h"
#include "devices/WS2812.h"
#include "Utils/dataFile.h"
#include <string>
#include <cstdint>

class LedCommandTask : public ITask {
public:
  LedCommandTask(std::shared_ptr<WS2812> device, const uint32_t* pattern_data, size_t pattern_size, int offsetjump, bool loop = false)
    : _device(device), _pattern_data(pattern_data), _pattern_size(pattern_size), _offsetjump(offsetjump), _loop(loop) {}

  bool ExecuteTask(TaskPID pid) override {
    if(!_device->setPattern(&_pattern_data[_current_offset], _device->getLEDCount())) { // Assuming each LED pattern is 4 bytes (e.g., RGB or RGBW)
      _is_playing = false;
      std::cout << "Failed to set LED pattern for device: " << _device->getName() << std::endl;
      return false;
    }

    _current_offset += _offsetjump;

    if(_pattern_size - _current_offset < _device->getLEDCount()) {
      if(_loop) {
        _current_offset = 0;
      } else {
        std::cout << "Finished playing LED pattern on device: " << _device->getName() << std::endl;
      }
    }
    
    return !isFinished();
  }

  const std::string getName() const override {
    return "LedPlayTask - " + _device->getName();
  }

  const std::string getDeviceName() const {
    return _device->getName();
  }

  const bool isFinished() const {
    if (!_is_playing) {
      return true; // If stopped, the task is finished
    }
    if (_loop) {
      return false; // If looping, the task is never finished
    }
    if (_pattern_size - _current_offset <= _device->getLEDCount()) {
      return true; // If we've reached the end of the pattern, the task is finished
    }
    return false;
  }

  void stop() {
    _is_playing = false;
  }

private:
  std::shared_ptr<WS2812> _device;
  const uint32_t* _pattern_data;
  size_t _pattern_size;
  int _offsetjump;
  bool _loop;
  int _current_offset = 0;
  bool _is_playing = true;
};

class LedCommand : public ICommand {
public:
  // Constructor
  LedCommand(Mainloop &mainloop, const Console &console, DeviceRepository &deviceRepo) : _mainloop(mainloop), _console(console), _deviceRepo(deviceRepo) {}

  // Returns the name of the command
  const std::string getName() const override {
    return "led";
  }

  const std::string getHelp() const override {
    return "Usage: led <deviceName> show <filename> [<offset>]\n"
           "       led <deviceName> play <filename> [<speed>]\n"
           "       led <deviceName> loop <filename> [<speed>]\n"
           "       led <deviceName> stop\n\n"
           "       Displays the contents of the specified file on the LED device.";
  }

  // Executes the command
  int execute(const std::vector<std::string> &args) override {
    for (auto it = _signalTasks.begin(); it != _signalTasks.end();) {
      if ((*it)->isFinished()) {
        it = _signalTasks.erase(it);
      } else {
        ++it;
      }
    }

    if (args.size() < 3) {
      std::cout << getHelp() << std::endl;
      return -1; // Return -1 to indicate failure
    }

    auto device = _deviceRepo.getDevice<WS2812>("WS2812", args[1]);
    if (!device) {
      std::cout << "Device not found: " << args[1] << std::endl;
      return -1; // Return -1 to indicate failure
    }

    std::unique_ptr<dataFileReader> reader;
    if (args.size() >= 4) {
      reader = std::make_unique<dataFileReader>(_console.currentDirectory, args[3]);
      if (!reader->isExpectedFile("LEDP")) {
        std::cout << "Invalid file header: " << args[3] << std::endl;
        return -1; // Return -1 to indicate failure
      }
    }

    int parameter = 0;
    if (args.size() >= 5) {
      parameter = std::strtol(args[4].c_str(), nullptr, 10);
      if (parameter <= 0) {
        std::cout << "Invalid parameter: " << args[4] << std::endl;
        return -1; // Return -1 to indicate failure
      }
    }

    if (args[2] == "show") {
      const uint32_t* pattern_data = nullptr;
      size_t pattern_size = 0;
      int offset = parameter;

      auto current = reader->start();
      while(current != nullptr && pattern_size == 0 && current != reader->end()) {
        if(reader->getFieldSignature(current) == 0xA470 /*dat*/) {
          pattern_size = reader->getDataSize(current) / 4; // Assuming each LED pattern is 4 bytes (e.g., RGB or RGBW)
          pattern_data = (const uint32_t*)reader->getFieldData(current);
        }else if(reader->getFieldSignature(current) == 0xAF96 /*jmp*/){
          const uint16_t* jump_data = reinterpret_cast<const uint16_t*>(reader->getFieldData(current));
          offset *= *jump_data;
        }
        current = reader->next(current);
      }

      if(pattern_size - offset < device->getLEDCount()) {
        std::cout << "The Offset is too large for the available pattern size." << std::endl;
        return -1; // Return -1 to indicate failure
      }
      
      if(!device->setPattern(&pattern_data[offset], device->getLEDCount())) { // Assuming each LED pattern is 4 bytes (e.g., RGB or RGBW)
        std::cout << "Failed to set LED pattern." << std::endl;
        return -1; // Return -1 to indicate failure
      }
      return 0; // Return 0 to indicate success
    } else if (args[2] == "play" || args[2] == "loop") {
      const uint32_t* pattern_data = nullptr;
      size_t pattern_size = 0;
      int speed = 0;
      int offset_jump = 0;

      auto current = reader->start();
      while(current != nullptr && pattern_size == 0 && current != reader->end()) {
        if(reader->getFieldSignature(current) == 0xA470 /*dat*/) {
          pattern_size = reader->getDataSize(current) / 4; // Assuming each LED pattern is 4 bytes (e.g., RGB or RGBW)
          pattern_data = (const uint32_t*)reader->getFieldData(current);
        }else if(reader->getFieldSignature(current) == 0x40DC /*tim*/){
          const uint16_t* timing_data = reinterpret_cast<const uint16_t*>(reader->getFieldData(current));
          speed = *timing_data;
        }else if(reader->getFieldSignature(current) == 0xAF96 /*jmp*/){
          const uint16_t* jump_data = reinterpret_cast<const uint16_t*>(reader->getFieldData(current));
          offset_jump = *jump_data;
        }
        current = reader->next(current);
      }

      if(parameter > 0) {
        speed = parameter;
      }

      bool loop = (args[2] == "loop");

      auto task = std::make_unique<LedCommandTask>(device, pattern_data, pattern_size, offset_jump, loop);
      _mainloop.registerTimedTask(task.get(), speed);
      _signalTasks.push_back(std::move(task));

      return 0;
    } else if (args[2] == "stop") {
      for (auto it = _signalTasks.begin(); it != _signalTasks.end();) {
        if ((*it)->getDeviceName() == device->getName()) {
          (*it)->stop();
        }
        ++it;
      }
      return 0;
    }
    std::cout << "Invalid action: " << args[2] << std::endl;
    std::cout << getHelp() << std::endl;
    return -1;
  }

private:
  Mainloop &_mainloop; // Reference to the mainloop object
  const Console &_console; // Reference to the console object
  DeviceRepository &_deviceRepo; // Reference to the device repository

  std::vector<std::unique_ptr<LedCommandTask>> _signalTasks; // Store active signal tasks for management
};