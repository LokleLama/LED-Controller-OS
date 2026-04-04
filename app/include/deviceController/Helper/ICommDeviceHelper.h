#pragma once

#include "devices/UARTDevice.h"
#include "devices/USBUARTDevice.h"
#include "devices/PassthroughMonitor.h"

class ICommDeviceHelper {
public:
    static std::shared_ptr<ICommDevice> findCommDevice(const std::string& name, DeviceRepository& deviceRepo) {
        auto uart_device = deviceRepo.getDevice<UARTDevice>("UART", name);
        if (uart_device && uart_device->getStatus() == IDevice::DeviceStatus::Initialized) {
            return uart_device;
        }
        auto usb_uart_device = deviceRepo.getDevice<USBUARTDevice>("USBUART", name);
        if (usb_uart_device && usb_uart_device->getStatus() == IDevice::DeviceStatus::Initialized) {
            return usb_uart_device;
        }
        auto monitor_device = deviceRepo.getDevice<PassthroughMonitor::MonitorDevice>("MonitorDevice", name);
        if (monitor_device && monitor_device->getStatus() == IDevice::DeviceStatus::Initialized) {
            return monitor_device;
        }
        return nullptr;
    }
};