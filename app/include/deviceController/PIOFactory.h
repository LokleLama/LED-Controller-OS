#pragma once

#include "deviceController/DeviceRepository.h"
#include "devices/PIODevice.h"

#include <memory>
#include <string>
#include <vector>

#include <cstdint>

class PIOFactory : public IDeviceFactory {
public:
    PIOFactory() = default;
    
    const Category getCategory() const override { return Category::PIO; }
    const std::vector<std::string> getDeviceNames() const override {
        static std::vector<std::string> names = {"PIO0",  "PIO1"};
        return names;
    }
    std::shared_ptr<IDevice> createDevice(const std::string& name, const std::vector<std::string>& params) override {
        int pio_number = std::stoi(name.substr(3));

        if (pio_number < 0 || pio_number >= 2 || claimed_sms[pio_number] >= 4) {
            return nullptr;
        }

        auto pio_device = std::make_shared<PIODevice>(pio_number);
        if (pio_device->getStatus() != IDevice::DeviceStatus::Initialized) {
            return nullptr;
        }
        claimed_sms[pio_number]++;
        return pio_device;
    }

private:
    uint8_t claimed_sms[2] = {0, 0};
};