#pragma once

#include "deviceController/DeviceRepository.h"
#include "devices/GPIODevice.h"

#include "VariableStore/VariableStore.h"
#include "VariableStore/SpecialVariables/GPIOVaraible.h"
#include "Utils/ValueConverter.h"

#include <memory>
#include <string>

class GPIOFactory : public IDeviceFactory {
public:
    GPIOFactory(DeviceRepository& deviceRepo) : _deviceRepo(deviceRepo) {}
    
    const Category getCategory() const override { return Category::GPIO; }
    const std::vector<std::string> getDeviceNames() const override {
        static std::vector<std::string> names = {"GPIO"};
        return names;
    }
    const std::string& getParameterInfo() const override{
        static std::string empty = "<GPIONumber> <Configuration> [name]\n"
                                   "  GPIONumber:     Number of the GPIO device to use (e.g.: 0 for GPIO0)\n"
                                   "  Configuration:  Configuration of the GPIO device.\n"
                                   "                  Possible values:\n"
                                   "                    in or INPUT\n"
                                   "                    out or OUTPUT\n"
                                   "                    pullup or INPUT_PULLUP\n"
                                   "                    pulldown or INPUT_PULLDOWN\n"
                                   "                    open_drain or OUTPUT_OPEN_DRAIN\n"
                                   "                    open_source or OUTPUT_OPEN_SOURCE\n"
                                   "  name:           Optional custom name for the GPIO variables, the device will use 'GPIO<GPIONumber>'";
        return empty;
    }
    std::shared_ptr<IDevice> createDevice(const std::string& name, const std::vector<std::string>& params) override {
        if (params.size() < 2) {
            return nullptr;
        }
        int gpio_number = std::strtol(params[0].c_str(), nullptr, 0);
        GPIODevice::GPIOConfiguration config = GPIODevice::parseConfiguration(params[1]);
        if (config == GPIODevice::GPIOConfiguration::INVALID) {
            std::cout << "Invalid GPIO configuration: " << params[1] << std::endl;
            return nullptr;
        }

        auto device = std::make_shared<GPIODevice>(gpio_number, config);

        std::string variable_name = device->getName();
        if (params.size() >= 3) {
            variable_name = params[2];
        }

        auto variable = std::make_shared<GPIOVariable>(variable_name, device);
        VariableStore::getInstance().registerVariable(variable);
        return device;
    }

private:
    DeviceRepository& _deviceRepo;
};
