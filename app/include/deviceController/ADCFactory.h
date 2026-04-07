#pragma once

#include "deviceController/DeviceRepository.h"
#include "devices/ADCDevice.h"
#include "VariableStore/VariableStore.h"
#include "VariableStore/SpecialVariables/CPUTemperature.h"

#include <memory>
#include <string>
#include <vector>

#include <cstdint>

class ADCFactory : public IDeviceFactory {
public:
    ADCFactory() = default;
    
    const Category getCategory() const override { return Category::ADC; }
    const std::vector<std::string> getDeviceNames() const override {
        static std::vector<std::string> names = {"ADC"};
        return names;
    }

    const std::string& getParameterInfo() const override {
        static std::string info = "<adc_channel> [readout_intervall] [name] [readout_variable_name]\n"
                                  "  adc_channel:            ADC channel number (0-4)\n"
                                  "  readout_intervall:      Interval for ADC readout in milliseconds (default: 100)\n"
                                  "  name:                   Optional custom name for the ADC device (default: 'ADCDevice')\n"
                                  "  readout_variable_name:  Name of the variable to store ADC readout (default: auto-generated)";
        return info;
    }

    std::shared_ptr<IDevice> createDevice(const std::string& name, const std::vector<std::string>& params) override {
        if (params.size() < 1) {
            std::cout << "Not enough parameters for ADC device. Required: adc_channel." << std::endl;
            return nullptr;
        }
        int adc_channel = std::stoi(params[0]);
        int readout_intervall = 100;
        if (params.size() >= 2) {
            readout_intervall = std::stoi(params[1]);
        }

        std::string device_name = "ADC.Channel" + std::to_string(adc_channel);
        if (params.size() >= 3) {
            device_name = params[2];
        }

        std::string readout_variable_name = "adc.ch" + std::to_string(adc_channel);
        if(adc_channel == 4) {
            readout_variable_name = "cpu.temperature";
        }
        if (params.size() >= 4) {
            readout_variable_name = params[3];
        }

        auto& varStore = VariableStore::getInstance();
        std::shared_ptr<IVariable> readout_var = nullptr;
        if(adc_channel == 4) {
            readout_var = varStore.registerVariable(std::make_shared<CPUTemperature>(readout_variable_name));
        } else {
            readout_var = varStore.addVariable(readout_variable_name, static_cast<int>(0));
        }
        readout_var->setSystemVariable();

        auto adc_device = std::make_shared<ADCDevice>(device_name, adc_channel, readout_var, readout_intervall);
        if (adc_device->getStatus() != IDevice::DeviceStatus::Initialized) {
            return nullptr;
        }

        return adc_device;
    }
};