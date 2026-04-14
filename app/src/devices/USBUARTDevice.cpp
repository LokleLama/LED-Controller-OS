#include "devices/USBUARTDevice.h"
#include "Utils/ValueConverter.h"
#include "tusb.h"


USBUARTDevice::USBUARTDevice(int interface_number) : _interface_number(interface_number) {
    Mainloop::getInstance().registerRegularTask(getName() + ".Worker", [this](TaskPID) { return ExecuteTask(); });
    _status = DeviceStatus::Initialized;
}

bool USBUARTDevice::ExecuteTask() {
    tud_task();

    if (_irq_signal != 0 && tud_cdc_n_available(_interface_number) != 0) {
        Mainloop::getInstance().triggerSignal(_irq_signal);
    }
    return true;
}

int USBUARTDevice::send(const uint8_t* data, size_t length) {
    auto ret = tud_cdc_n_write(_interface_number, data, length);
    tud_cdc_n_write_flush(_interface_number);
    return ret;
}

int USBUARTDevice::dataAvailable() {
    return tud_cdc_n_available(_interface_number);
}

int USBUARTDevice::receive(uint8_t* buffer, size_t length) {
    return tud_cdc_n_read(_interface_number, buffer, length);
}

const std::string USBUARTDevice::getDetails() const {
    if(_irq_signal != 0) {
        return "USBUART Device (IRQ Signal: " + ValueConverter::toString(_irq_signal, IntegerStringFormat::HEX) + ")";
    }
    return "USBUART Device";
}

bool USBUARTDevice::registerDataReceivedCallback(Mainloop::Function func, Signal signal) {
    if(_irq_signal != 0) {
        return false;
    }
    if(signal == 0) {
        signal = 0x55534255;
    }
    _irq_signal = signal;
    Mainloop::getInstance().registerSignalTask(getName() + ".DataReceived", func, _irq_signal);

    return true;
}

