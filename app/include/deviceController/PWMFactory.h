#pragma once

#include "deviceController/DeviceRepository.h"
#include "devices/PWMDevice.h"

#include <memory>
#include <string>
#include <vector>

#include <cstdint>
#include <cstdlib>
#include <iostream>

class PWMFactory : public IDeviceFactory {
public:
    PWMFactory() = default;

    const Category getCategory() const override { return Category::Communication; }
    const std::vector<std::string> getDeviceNames() const override {
        static std::vector<std::string> names = {"PWM"};
        return names;
    }
    const std::string& getParameterInfo() const override {
        static std::string info = "<gpio_pin> <frequency_hz> [phase_correct]\n"
                                  "  gpio_pin:                GPIO used for PWM output\n"
                                  "  frequency_hz:            PWM frequency in Hz (default: 1000)\n"
                                  "  phase_correct:           0/1 to disable/enable phase-correct mode (default: 1)";
        return info;
    }

    std::shared_ptr<IDevice> createDevice(const std::string& name, const std::vector<std::string>& params) override {
        if (params.size() < 1) {
            return nullptr;
        }

        uint8_t gpio_pin = static_cast<uint8_t>(std::strtol(params[0].c_str(), nullptr, 0));
        int frequency_hz = 1000;
        bool phase_correct = true;

        if (params.size() >= 2) {
            frequency_hz = static_cast<int>(std::strtol(params[1].c_str(), nullptr, 0));
        }
        if (params.size() >= 3) {
            phase_correct = (std::strtol(params[2].c_str(), nullptr, 0) != 0);
        }

        auto device = std::make_shared<PWMDevice>(gpio_pin, frequency_hz, phase_correct);
        if (device->getStatus() != IDevice::DeviceStatus::Initialized) {
            std::cout << "Failed to initialize PWM on GPIO" << static_cast<int>(gpio_pin) << std::endl;
            return nullptr;
        }

        if (!setupVariable(device)) {
            std::cout << "Failed to setup variables for PWM on GPIO" << static_cast<int>(gpio_pin) << std::endl;
            return nullptr;
        }

        return device;
    }

private:
    bool setupVariable(std::shared_ptr<PWMDevice> device) {
        auto& var_store = VariableStore::getInstance();
        std::string var_frequency = device->getName() + ".frequency";
        var_store.addVariable(var_frequency, device->getFrequency())->setSystemVariable();
        var_store.registerCallback(var_frequency, [device](const std::string& key, const std::string& value) {
            auto success = device->configurePWM(ValueConverter::toInt(value));
            std::cout << "PWM Frequency changed to " << device->getFrequency()
                  << " Hz (requested: " << value << ")" << std::endl;
            return success;
        });

        std::string var_duty = device->getName() + ".duty";
        var_store.addVariable(var_duty, 50.0f)->setSystemVariable();
        var_store.registerCallback(var_duty, [device](const std::string& key, const std::string& value) {
            auto fvalue = std::strtof(value.c_str(), nullptr);
            if (fvalue < 0.0f || fvalue > 100.0f) {
                return false;
            }
            auto success = device->setLevel(device->getWrap() * (fvalue / 100.0f));
            std::cout << "PWM Duty changed to " << fvalue << "%" << std::endl;
            return success;
        });

        return true;
    }
};
