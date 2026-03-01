#pragma once

#include "deviceController/DeviceRepository.h"
#include "devices/UARTDevice.h"

#include <memory>
#include <string>
#include <vector>

#include <cstdint>

class UARTFactory : public IDeviceFactory {
public:
    UARTFactory() = default;
    
    const Category getCategory() const override { return Category::Communication; }
    const std::vector<std::string> getDeviceNames() const override {
        static std::vector<std::string> names = {"UART0",  "UART1"};
        return names;
    }
    const std::string& getParameterInfo() const override{
        static std::string empty = "<tx_pin> <rx_pin> [baud_rate]\n"
                                   "  tx_pin:         TX pin number on the UART device to use\n"
                                   "  rx_pin:         RX pin number on the UART device to use\n"
                                   "  baud_rate:      Baud rate for the UART communication (default: 115200)";
        return empty;
    }
    std::shared_ptr<IDevice> createDevice(const std::string& name, const std::vector<std::string>& params) override {
        int uart_number = std::stoi(name.substr(4));

        if (uart_number < 0 || uart_number >= 2) {
            return nullptr;
        }

        if (params.size() < 2) {
            return nullptr;
        }
        uint8_t tx_pin = static_cast<uint8_t>(std::stoi(params[0]));
        uint8_t rx_pin = static_cast<uint8_t>(std::stoi(params[1]));
        uint baud_rate = 115200;
        if (params.size() >= 3) {
            baud_rate = static_cast<uint>(std::stoi(params[2]));
        }

        auto uart_device = std::make_shared<UARTDevice>(uart_number, tx_pin, rx_pin, baud_rate);
        if (uart_device->getStatus() != IDevice::DeviceStatus::Initialized) {
            return nullptr;
        }
        return uart_device;
    }
};