#pragma once

#include "deviceController/DeviceRepository.h"
#include "devices/ICommDevice.h"
#include "devices/Loopback.h"
#include "devices/Passthrough.h"
#include "deviceController/Helper/ICommDeviceHelper.h"

#include <memory>
#include <string>
#include <vector>

#include <cstdint>
#include <iostream>

class CommRouterFactory : public IDeviceFactory {
public:
    CommRouterFactory(DeviceRepository& deviceRepo) : _deviceRepo(deviceRepo) {}
    
    const Category getCategory() const override { return Category::Communication; }
    const std::vector<std::string> getDeviceNames() const override {
        static std::vector<std::string> names = {"Loopback", "Passthrough"};
        return names;
    }
    const std::string& getParameterInfo() const override{
        static std::string description = "Loopback <ICommDeviceName> [name] [buffer-size]\n"
                                         "  ICommDeviceName:    Name of the IComm device to use (e.g.: USBUART-0)\n"
                                         "  name:               Optional unique name for the device (default: auto-generated)\n"
                                         "  buffer-size:        Optional buffer size for the loopback (default: 4x IComm device buffer size)\n\n"
                                         "Passthrough <ICommDeviceNameA> <ICommDeviceNameB> [name] [buffer-size]\n"
                                         "  ICommDeviceNameA:   Name of the first IComm device to use (e.g.: USBUART-0)\n"
                                         "  ICommDeviceNameB:   Name of the second IComm device to use (e.g.: UART-1)\n"
                                         "  name:               Optional unique name for the device (default: auto-generated)\n"
                                         "  buffer-size:        Optional buffer size for the passthrough (default: 4x IComm device buffer size)\n";

        return description;
    }
    std::shared_ptr<IDevice> createDevice(const std::string& name, const std::vector<std::string>& params) override {
        if (name == "Loopback") {
            return createLoopback(name, params);
        } else if (name == "Passthrough") {
            return createPassthrough(name, params);
        }

        return nullptr;
    }

private:
    DeviceRepository& _deviceRepo;
    uint8_t _number = 0;

    std::shared_ptr<IDevice> createPassthrough(const std::string& name, const std::vector<std::string>& params) {
        if (params.size() < 2) {
            return nullptr;
        }
        std::shared_ptr<ICommDevice> comm_device_a = ICommDeviceHelper::findCommDevice(params[0], _deviceRepo);
        if (!comm_device_a) {
            std::cout << "Invalid IComm device: " << params[0] << std::endl;
            return nullptr;
        }
        std::shared_ptr<ICommDevice> comm_device_b = ICommDeviceHelper::findCommDevice(params[1], _deviceRepo);
        if (!comm_device_b) {
            std::cout << "Invalid IComm device: " << params[1] << std::endl;
            return nullptr;
        }
        std::string device_name;
        if (params.size() >= 3) {
            device_name = params[2];
        } else {
            device_name = "Passthrough-" + std::to_string(_number);
        }
        _number++;
        int buffer_size = comm_device_a->getBufferSize() * 4;
        if (params.size() >= 4) {
            buffer_size = std::stoi(params[3]);
        }else{
            int buffer_size_b = comm_device_b->getBufferSize() * 4;
            if(buffer_size_b > buffer_size) {
                buffer_size = buffer_size_b;
            }
        }

        auto passthrough_device = std::make_shared<Passthrough>(comm_device_a, comm_device_b, device_name, buffer_size);
        if (passthrough_device->getStatus() != IDevice::DeviceStatus::Initialized) {
            std::cout << "Failed to initialize Passthrough device: " << device_name << std::endl;
            return nullptr;
        }

        if(!comm_device_a->assignToUser(passthrough_device)){
            std::cout << "Failed to assign " << comm_device_a->getName() << " to Passthrough device: " << device_name << std::endl;
            return nullptr;
        }
        
        if(!comm_device_b->assignToUser(passthrough_device)){
            std::cout << "Failed to assign " << comm_device_b->getName() << " to Passthrough device: " << device_name << std::endl;
            return nullptr;
        }

        return passthrough_device;
    }

    std::shared_ptr<IDevice> createLoopback(const std::string& name, const std::vector<std::string>& params) {
        if (params.size() < 1) {
            return nullptr;
        }
        std::shared_ptr<ICommDevice> comm_device = ICommDeviceHelper::findCommDevice(params[0], _deviceRepo);
        if (!comm_device) {
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
        int buffer_size = comm_device->getBufferSize() * 4;
        if (params.size() >= 3) {
            buffer_size = std::stoi(params[2]);
        }

        auto loopback_device = std::make_shared<Loopback>(comm_device, device_name, buffer_size);
        if (loopback_device->getStatus() != IDevice::DeviceStatus::Initialized) {
            std::cout << "Failed to initialize Loopback device: " << device_name << std::endl;
            return nullptr;
        }

        if(!comm_device->assignToUser(loopback_device)){
            std::cout << "Failed to assign " << comm_device->getName() << " to Loopback device: " << device_name << std::endl;
            return nullptr;
        }

        return loopback_device;
    }
};
