#pragma once

#include "deviceController/DeviceRepository.h"
#include "devices/UARTDevice.h"
#include "Utils/ValueConverter.h"

#include "VariableStore/VariableStore.h"

#include <memory>
#include <string>
#include <vector>

#include <cstdint>
#include <iostream>

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

        if(_opened[uart_number]) {
            std::cout << "UART" << uart_number << " is already open" << std::endl;
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
        setupVariable(uart_device, baud_rate);
        _opened[uart_number] = true;
        return uart_device;
    }

private:
    static bool _opened[2];

    bool setupVariable(std::shared_ptr<UARTDevice> device, int baudrate) {
        auto& variableStore = VariableStore::getInstance();

        auto baudvar = device->getName() + ".baud";
        variableStore.addVariable(baudvar, baudrate);
        variableStore.registerCallback(baudvar, [device](const std::string& key, const std::string& value) {
            auto success = device->setBaudRate(ValueConverter::toInt(value));
            std::cout << "Baud rate changed to " << device->getBaudRate()
                  << " (requested: " << value << ")" << std::endl;
            return success;
        });

        auto formatvar = device->getName() + ".format";
        variableStore.addVariable(formatvar, "8n1");
        variableStore.registerCallback(formatvar, [device](const std::string& key, const std::string& value) {
            if (value.size() < 3) {
                std::cout << "Invalid format value" << std::endl;
                return false;
            }
            int bits = value[0] - '0';
            char parity = value[1];
            int stopBits = value[2] - '0';

            if (bits < 5 || bits > 8 ||
                (parity != 'n' && parity != 'o' && parity != 'e') ||
                (stopBits != 1 && stopBits != 2)) {
                std::cout << "Invalid format value" << std::endl;
                return false;
            }

            int parityValue = 0;
            switch (parity) {
                case 'n': parityValue = UART_PARITY_NONE; break;
                case 'e': parityValue = UART_PARITY_EVEN; break;
                case 'o': parityValue = UART_PARITY_ODD; break;
                default:
                    std::cout << "Invalid format value" << std::endl;
                    return false;
            }

            device->setFormat(bits, stopBits, parityValue);
            std::cout << "Format changed to " << bits << parity << stopBits << std::endl;
            return false;
        });

        return true;
    }
};