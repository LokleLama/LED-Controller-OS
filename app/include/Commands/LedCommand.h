#pragma once

#include "../ICommand.h"
#include "deviceController/DeviceRepository.h"
#include "devices/WS2812.h"
#include <string>
#include <cstdint>

class LedCommand : public ICommand {
public:
  // Constructor
  LedCommand(const Console &console) : _console(console) {}

  // Returns the name of the command
  const std::string getName() const override {
    return "led";
  }

  const std::string getHelp() const override {
    return "Usage: led <deviceName> show <filename>\n"
           "       Displays the contents of the specified file on the LED device.";
  }

  // Executes the command
  int execute(const std::vector<std::string> &args) override {
    if (args.size() < 4) {
      std::cout << getHelp() << std::endl;
      return -1; // Return -1 to indicate failure
    }

    auto device = DeviceRepository::getInstance().getDevice<WS2812>("WS2812", args[1]);
    if (!device) {
      std::cout << "Device not found: " << args[1] << std::endl;
      return -1; // Return -1 to indicate failure
    }

    if (args[2] == "show") {
      auto file = _console.currentDirectory->openFile(args[3]);
      if (!file) {
        std::cout << "File not found: " << args[3] << std::endl;
        return -1; // Return -1 to indicate failure
      }
      auto data = file->getMemoryMappedAddress();
      
      if(!device->setPattern((uint32_t*)data, file->getSize() / 4)) { // Assuming each LED pattern is 4 bytes (e.g., RGB or RGBW)
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
};