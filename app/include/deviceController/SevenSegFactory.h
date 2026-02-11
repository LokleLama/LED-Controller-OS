#pragma once

#include "deviceController/DeviceRepository.h"
#include "devices/WS2812.h"
#include "devices/SevenSeg.h"

#include <memory>
#include <string>
#include <vector>

#include <cstdint>
#include <iostream>

class SevenSegFactory : public IDeviceFactory {
public:
    SevenSegFactory() = default;
    
    const Category getCategory() const override { return Category::UserInterface; }
    const std::vector<std::string> getDeviceNames() const override {
        static std::vector<std::string> names = {"7Seg"};
        return names;
    }
    const std::string& getParameterInfo() const override{
        static std::string empty = "<WS2812DeviceName> [name]\n"
                                   "  WS2812DeviceName:  Name of the WS2812 device to use (e.g.: WS2812.0)\n"
                                   "  name:              Optional unique name for the device (default: auto-generated)";
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
            device_name = "7Seg." + std::to_string(_number);
        }
        _number++;
        auto seven_seg_device = std::make_shared<SevenSeg>(ws2812_device, device_name);
        if (seven_seg_device->getStatus() != IDevice::DeviceStatus::Initialized) {
            std::cout << "Failed to initialize 7Seg device: " << device_name << std::endl;
            return nullptr;
        }
        if(!ws2812_device->assignToUser(seven_seg_device)){
            std::cout << "Failed to assign WS2812 device to 7Seg device: " << device_name << std::endl;
            return nullptr;
        }
        return seven_seg_device;
    }

private:
    uint8_t _number = 0;
};
