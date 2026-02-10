#pragma once

#include "deviceController/DeviceRepository.h"
#include "devices/PIODevice.h"
#include "devices/WS2812.h"

#include <memory>
#include <string>
#include <vector>

#include <cstdint>
#include <iostream>

class LEDFactory : public IDeviceFactory {
public:
    LEDFactory() = default;
    
    const Category getCategory() const override { return Category::Communication; }
    const std::vector<std::string> getDeviceNames() const override {
        static std::vector<std::string> names = {"WS2812"};
        return names;
    }
    const std::string& getParameterInfo() const override{
        static std::string empty = "<PIODeviceName> <pin> <num_leds> [bits_per_pixel] [frequency] [name]\n"
                                   "  PIODeviceName:  Name of the PIO device to use (e.g.: PIO0.SM0)\n"
                                   "  pin:            Pin number on the PIO device to use\n"
                                   "  num_leds:       Number of LEDs in the strip\n"
                                   "  bits_per_pixel: Number of bits per pixel, typically 24 for RGB or 32 for RGBW (default: 24)\n"
                                   "  frequency:      Signal frequency in Hz, typically 800000 for WS2812 (default: 800000)\n"
                                   "  name:           Optional unique name for the device (default: auto-generated)";
        return empty;
    }
    std::shared_ptr<IDevice> createDevice(const std::string& name, const std::vector<std::string>& params) override {
        if (params.size() < 3) {
            return nullptr;
        }
        std::string pio_device_name = params[0];
        auto pio_device = DeviceRepository::getInstance().getDevice<PIODevice>("PIO", pio_device_name);
        if (!pio_device || pio_device->getStatus() != IDevice::DeviceStatus::Initialized) {
            std::cout << "Invalid PIO device: " << pio_device_name << std::endl;
            return nullptr;
        }
        int pin = std::stoi(params[1]);
        int num_leds = std::stoi(params[2]);
        int bits_per_pixel = 24;
        float frequency = 800000;
        if (params.size() >= 4) {
            bits_per_pixel = std::stoi(params[3]);
        }
        if (params.size() >= 5) {
            frequency = std::stof(params[4]);
        }
        std::string device_name;
        if (params.size() >= 6) {
            device_name = params[5];
        }else{
            device_name = "WS2812." + std::to_string(_number);
        }
        _number++;
        auto led_device = std::make_shared<WS2812>(pio_device, pin, num_leds, bits_per_pixel, frequency, device_name);
        if (led_device->getStatus() != IDevice::DeviceStatus::Initialized) {
            std::cout << "Failed to initialize LED device: " << device_name << std::endl;
            return nullptr;
        }
        if(!pio_device->assignToUser(led_device)){
            std::cout << "Failed to assign PIO device to LED device: " << device_name << std::endl;
            return nullptr;
        }
        return led_device;
    }

private:
    uint8_t _number = 0;
};