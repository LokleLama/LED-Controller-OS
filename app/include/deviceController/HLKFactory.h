#pragma once

#include "deviceController/DeviceRepository.h"
#include "deviceController/Helper/ICommDeviceHelper.h"

#include "VariableStore/VariableStore.h"

#include "devices/HLKDevice.h"

#include <memory>
#include <string>
#include <vector>

#include <cstdint>
#include <iostream>

class HLKFactory : public IDeviceFactory {
public:
    HLKFactory(DeviceRepository& deviceRepo) : _deviceRepo(deviceRepo) {}

    const Category getCategory() const override { return Category::Protocol; }
    const std::vector<std::string> getDeviceNames() const override {
        static std::vector<std::string> names = {"HLKDevice"};
        return names;
    }
    const std::string& getParameterInfo() const override {
        static std::string info = "<comm_device_name> [name] [distance_variable_name] [presence_variable_name]\n"
                                   "  comm_device_name:       Name of the communication device (e.g., UART0) to which the HLK device is connected\n"
                                   "  name:                   Optional custom name for the HLK device (default: 'HLKDevice')\n"
                                   "  distance_variable_name: Name of the variable to store distance measurements (default: auto-generated)\n"
                                   "  presence_variable_name: Name of the variable to store presence detection state (default: auto-generated)";
        return info;
    }

    std::shared_ptr<IDevice> createDevice(const std::string& name, const std::vector<std::string>& params) override {
        if (params.size() < 1) {
            return nullptr;
        }
        std::shared_ptr<ICommDevice> comm_device = ICommDeviceHelper::findCommDevice(params[0], _deviceRepo);
        if (!comm_device) {
            std::cout << "Invalid communication device: " << params[0] << std::endl;
            return nullptr;
        }

        std::string device_name = "HLKDevice";
        if (params.size() >= 2) {
            device_name = params[1];
        }

        std::string distance_variable_name = device_name + ".distance";
        if (params.size() >= 3) {
            distance_variable_name = params[2];
        }

        std::string presence_variable_name = device_name + ".presence";
        if (params.size() >= 4) {
            presence_variable_name = params[3];
        }

        auto& varStore = VariableStore::getInstance();
        
        auto distance_var = varStore.addVariable(distance_variable_name, static_cast<float>(0.0));
        auto presence_var = varStore.addBoolVariable(presence_variable_name, false);

        auto hlk_device = std::make_shared<HLKDevice>(comm_device, distance_var, presence_var, device_name);
        if (hlk_device->getStatus() != IDevice::DeviceStatus::Initialized) {
            std::cout << "Failed to initialize HLKDevice: " << device_name << std::endl;
            return nullptr;
        }

        if(!comm_device->assignToUser(hlk_device)){
            std::cout << "Failed to assign communication device to HLKDevice: " << device_name << std::endl;
            return nullptr;
        }

        return hlk_device;
    }

private:
    DeviceRepository& _deviceRepo;
};
