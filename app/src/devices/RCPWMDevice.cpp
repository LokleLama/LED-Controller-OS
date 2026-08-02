#include "devices/RCPWMDevice.h"

#include "hardware/clocks.h"
#include "hardware/gpio.h"

#include <cmath>
#include <iostream>


RCPWMDevice::RCPWMDevice(uint8_t gpio_pin)
    : PWMDevice(gpio_pin, 50, true) {
    if (_status == DeviceStatus::Error) {
        return;
    }

    _millisecond_unit = static_cast<float>(_wrap) / 20.0f;
    _millisecond_unit_factor = _millisecond_unit / 100.0f;

    setPercent(50.0f);

    _status = DeviceStatus::Initialized;
}

bool RCPWMDevice::setPercent(float percent) {
    if (percent < 0.0f || percent > 100.0f) {
        return false;
    }

    uint16_t level = static_cast<uint16_t>(std::round(percent * _millisecond_unit_factor + _millisecond_unit));

    return PWMDevice::setLevel(level);
}

const std::string RCPWMDevice::getDetails() const {
    return getDetailsInternal("RCPWM");
}
