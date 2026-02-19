#pragma once

#include "deviceController/DeviceRepository.h"
#include "devices/WS2812.h"
#include "devices/SevenSeg.h"
#include "devices/dotMatrix5x5.h"

#include "VariableStore/VariableStore.h"

#include <memory>
#include <string>
#include <vector>

#include <cstdint>
#include <iostream>

class LEDDisplayFactory : public IDeviceFactory {
public:
    LEDDisplayFactory() = default;
    
    const Category getCategory() const override { return Category::UserInterface; }
    const std::vector<std::string> getDeviceNames() const override {
        static std::vector<std::string> names = {"7Seg", "dotMatrix5x5"};
        return names;
    }
    const std::string& getParameterInfo() const override{
        static std::string empty = "<WS2812DeviceName> [name] [start value]\n"
                                   "  WS2812DeviceName:  Name of the WS2812 device to use (e.g.: WS2812.0)\n"
                                   "  name:              Optional unique name for the device (default: auto-generated)\n"
                                   "  start value:       Optional initial value for the LED display (default: 00.00)";
        return empty;
    }
    std::shared_ptr<IDevice> createDevice(const std::string& name, const std::vector<std::string>& params) override {
        if (params.size() < 1) {
            return nullptr;
        }
        auto ws2812_device = DeviceRepository::getInstance().getDevice<WS2812>("WS2812", params[0]);
        if (!ws2812_device || ws2812_device->getStatus() != IDevice::DeviceStatus::Initialized) {
            std::cout << "Invalid WS2812 device: " << params[0] << std::endl;
            return nullptr;
        }
        std::string device_name;
        if (params.size() >= 2) {
            device_name = params[1];
        } else {
            device_name = name + "." + std::to_string(_number);
        }
        std::string start_value = "00.00";
        if (params.size() >= 3) {
            start_value = params[2];
        }
        _number++;

        std::shared_ptr<IDisplayDevice> new_display_device;
        if(name == "7Seg") {
            auto seven_seg_device = std::make_shared<SevenSeg>(ws2812_device, device_name, start_value);
            if (seven_seg_device->getStatus() != IDevice::DeviceStatus::Initialized) {
                std::cout << "Failed to initialize 7Seg device: " << device_name << std::endl;
                return nullptr;
            }
            if(!setupVariable(seven_seg_device, start_value)){
                std::cout << "Failed to setup variable for 7Seg device: " << device_name << std::endl;
            }
            new_display_device = seven_seg_device;
        }else if(name == "dotMatrix5x5"){
            new_display_device = std::make_shared<dotMatrix5x5>(ws2812_device, device_name, start_value);
            if (new_display_device->getStatus() != IDevice::DeviceStatus::Initialized) {
                std::cout << "Failed to initialize dotMatrix5x5 device: " << device_name << std::endl;
                return nullptr;
            }
        }else{
            std::cout << "Invalid device name: " << name << std::endl;
            return nullptr;
        }
        
        if(!ws2812_device->assignToUser(new_display_device)){
            std::cout << "Failed to assign WS2812 device to " << name << " device: " << device_name << std::endl;
            return nullptr;
        }
        return new_display_device;
    }

private:
    uint8_t _number = 0;

    bool setupVariable(std::shared_ptr<SevenSeg> device, const std::string& defaultValue){
        auto& variableStore = VariableStore::getInstance();

        variableStore.addVariable(device->getName() + ".value", defaultValue);
        variableStore.registerCallback(device->getName() + ".value", [device](const std::string& key, const std::string& value) {
            device->setValue(value);
            return true;
        });

        variableStore.addVariable(device->getName() + ".color", 0x03030303);
        variableStore.registerCallback(device->getName() + ".color", [device](const std::string& key, const std::string& value) {
            device->setColor(std::stoul(value, nullptr, 0));
            return true;
        });

        return true;
    }
};
