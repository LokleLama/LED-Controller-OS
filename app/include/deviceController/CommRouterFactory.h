#pragma once

#include "deviceController/DeviceRepository.h"
#include "devices/UARTDevice.h"
#include "devices/USBUARTDevice.h"
#include "devices/Loopback.h"

#include <memory>
#include <string>
#include <vector>

#include <cstdint>
#include <iostream>

class CommRouterFactory : public IDeviceFactory {
public:
    CommRouterFactory(DeviceRepository& deviceRepo) : _deviceRepo(deviceRepo) {}
    
    const Category getCategory() const override { return Category::UserInterface; }
    const std::vector<std::string> getDeviceNames() const override {
        static std::vector<std::string> names = {"Loopback"};
        return names;
    }
    const std::string& getParameterInfo() const override{
        static std::string empty = "<ICommDeviceName> [name] [buffer-size]\n"
                                   "  ICommDeviceName:    Name of the IComm device to use (e.g.: USBUART-0)\n"
                                   "  name:               Optional unique name for the device (default: auto-generated)\n"
                                   "  buffer-size:        Optional buffer size for the loopback (default: 64)\n";

        return empty;
    }
    std::shared_ptr<IDevice> createDevice(const std::string& name, const std::vector<std::string>& params) override {
        if (params.size() < 1) {
            return nullptr;
        }
        std::shared_ptr<ICommDevice> comm_device = _deviceRepo.getDevice<UARTDevice>("UART", params[0]);
        if(!comm_device) {
            comm_device = _deviceRepo.getDevice<USBUARTDevice>("USBUART", params[0]);
        }
        if (!comm_device || comm_device->getStatus() != IDevice::DeviceStatus::Initialized) {
            std::cout << "Invalid IComm device: " << params[0] << std::endl;
            return nullptr;
        }
        std::string device_name;
        if (params.size() >= 2) {
            device_name = params[1];
        } else {
            device_name = "Loopback-" + std::to_string(_number);
        }
        _number++;
        int buffer_size = 64;
        if (params.size() >= 3) {
            buffer_size = std::stoi(params[2]);
        }

        auto loopback_device = std::make_shared<Loopback>(comm_device, device_name, buffer_size);
        if (loopback_device->getStatus() != IDevice::DeviceStatus::Initialized) {
            std::cout << "Failed to initialize Loopback device: " << device_name << std::endl;
            return nullptr;
        }

        return loopback_device;
    }

private:
    DeviceRepository& _deviceRepo;
    uint8_t _number = 0;
};
