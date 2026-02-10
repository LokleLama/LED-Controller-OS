#pragma once

#include "../ICommand.h"
#include "../deviceController/DeviceRepository.h"
#include "Console.h"
#include <iostream>

class DeviceCommand : public ICommand {
public:
  // Constructor
  DeviceCommand(IVariableStore &store) : _store(store) {}
  // Returns the name of the command
  const std::string getName() const override { return "device"; }

  const std::string getHelp() const override {
    return "Usage: device <command>\n"
           "          list                        - Lists all available devices\n"
           "          list open                   - Lists all opened devices\n"
           "          param <name>                - Displays parameter information for the specified device\n"
           "          info <name>                 - Displays information about the specified device\n"
           "          open <name> [parameters]    - Opens the specified device (opening a device will set the variable 'lastOpenedDevice')\n";
  }

  // Executes the command
  int execute(const std::vector<std::string> &args) override {
    if (args.size() < 2) {
      std::cout << getHelp() << std::endl;
      return -1; // Return -1 to indicate failure
    }
    if (args[1] == "list") {
      return executeList(args);
    }
    if (args[1] == "param") {
      return executeParam(args);
    }
    if (args[1] == "info") {
      return executeInfo(args);
    }
    if (args[1] == "open") {
      return executeOpen(args);
    }
    std::cout << getHelp() << std::endl;
    return -1; // Return -1 to indicate failure
  }

private:
  IVariableStore &_store;
  
  int executeList(const std::vector<std::string> &args) {
    if (args.size() > 2 && args[2] == "open") {
      auto devices = DeviceRepository::getInstance().getDevices();
      std::cout << "Opened devices:" << std::endl;
      for (const auto& device : devices) {
        std::cout << "  " << device->getName();
        std::cout << " (" << IDevice::DeviceStatusToString(device->getStatus()) << ")";
        if(device->getUser()) {
          std::cout << " < " << device->getUser()->getName();
        }
        std::cout << std::endl;
      }
      return 0; // Return 0 to indicate success
    }
    
    auto deviceNames = DeviceRepository::getInstance().getAvailableDeviceNames();
    std::cout << "Available devices:" << std::endl;
    int count = 0;
    for (const auto& name : deviceNames) {
      std::cout << "  " << name;
      count++;
      if (count == 5) {
        std::cout << std::endl; // New line after every 5 devices for better readability
        count = 0;
      }
    }
    std::cout << std::endl; // Ensure the last line is terminated
    return 0; // Return 0 to indicate success
  }

  int executeParam(const std::vector<std::string> &args) {
    if (args.size() < 3) {
      std::cout << getHelp() << std::endl;
      return -1; // Return -1 to indicate failure
    }
    auto paramInfo = DeviceRepository::getInstance().getParameterInfo(args[2]);
    std::cout << "Parameter info for " << args[2] << ": " << paramInfo << std::endl;
    return 0; // Return 0 to indicate success
  }

  int executeInfo(const std::vector<std::string> &args) {
    if (args.size() < 3) {
      std::cout << getHelp() << std::endl;
      return -1; // Return -1 to indicate failure
    }
    auto device = DeviceRepository::getInstance().getDeviceByName(args[2]);
    if (!device) {
      std::cout << "Device not found: " << args[2] << std::endl;
      return -1; // Return -1 to indicate failure
    }
    std::cout << "Device name: " << device->getName() << std::endl;
    std::cout << "Status:      " << IDevice::DeviceStatusToString(device->getStatus()) << std::endl;
    std::cout << "Type:        " << device->getType() << std::endl;
    std::cout << "Details:     " << device->getDetails() << std::endl;
    return 0; // Return 0 to indicate success
  }

  int executeOpen(const std::vector<std::string> &args) {
    if (args.size() < 3) {
      std::cout << getHelp() << std::endl;
      return -1; // Return -1 to indicate failure
    }
    std::string deviceName = args[2];
    std::vector<std::string> params(args.begin() + 3, args.end());
    auto device = DeviceRepository::getInstance().createDevice(deviceName, params);
    if (!device) {
      std::cout << "Failed to open device: " << deviceName << std::endl;
      return -1; // Return -1 to indicate failure
    }
    std::cout << "Device opened: " << device->getName() << std::endl;
    if(!_store.setVariable("lastOpenedDevice", device->getName())) {
      _store.addVariable("lastOpenedDevice", device->getName());
    }
    return 0; // Return 0 to indicate success
  }
};