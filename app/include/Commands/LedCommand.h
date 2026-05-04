#pragma once

#include "ICommand.h"
#include "deviceController/DeviceRepository.h"
#include "devices/WS2812.h"
#include "Utils/dataFile.h"
#include <string>
#include <cstdint>

class LedCommand : public ICommand {
public:
  // Constructor
  LedCommand(const Console &console, DeviceRepository &deviceRepo) : _console(console), _deviceRepo(deviceRepo) {}

  // Returns the name of the command
  const std::string getName() const override {
    return "led";
  }

  const std::string getHelp() const override {
    return "Usage: led <deviceName> show <filename> [offset]\n"
           "       Displays the contents of the specified file on the LED device.";
  }

  // Executes the command
  int execute(const std::vector<std::string> &args) override {
    if (args.size() < 4) {
      std::cout << getHelp() << std::endl;
      return -1; // Return -1 to indicate failure
    }

    auto device = _deviceRepo.getDevice<WS2812>("WS2812", args[1]);
    if (!device) {
      std::cout << "Device not found: " << args[1] << std::endl;
      return -1; // Return -1 to indicate failure
    }

    if (args[2] == "show") {
      auto reader = std::make_unique<dataFileReader>(_console.currentDirectory, args[3]);
      if (!reader->isExpectedFile("LEDP")) {
        std::cout << "Invalid file header: " << args[3] << std::endl;
        return -1; // Return -1 to indicate failure
      }

      const uint32_t* pattern_data = nullptr;
      size_t pattern_size = 0;
      int offset = 0;

      if (args.size() >= 5) {
        offset = std::strtol(args[4].c_str(), nullptr, 10);
        if (offset < 0) {
          std::cout << "Invalid offset: " << args[4] << std::endl;
          return -1; // Return -1 to indicate failure
        }
      }

      auto current = reader->start();
      while(current != nullptr && pattern_size == 0 && current != reader->end()) {
        if(reader->getFieldSignature(current) == 0xA470 /*dat*/) {
          pattern_size = reader->getDataSize(current) / 4; // Assuming each LED pattern is 4 bytes (e.g., RGB or RGBW)
          pattern_data = (const uint32_t*)reader->getFieldData(current);
        }else if(reader->getFieldSignature(current) == 0xAF96 /*jmp*/){
          const uint16_t* jump_data = reinterpret_cast<const uint16_t*>(reader->getFieldData(current)); 
          std::cout << "Offset: " << offset << " Applying jump multiplier: " << *jump_data << std::endl;
          offset *= *jump_data;
          std::cout << "New Offset: " << offset << std::endl;
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
    }
    std::cout << "Invalid action: " << args[2] << std::endl;
    std::cout << getHelp() << std::endl;
    return -1;
  }

private:
  const Console &_console; // Reference to the console object
  DeviceRepository &_deviceRepo; // Reference to the device repository
};