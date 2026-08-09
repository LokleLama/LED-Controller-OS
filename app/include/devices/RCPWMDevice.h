#pragma once

#include "devices/PWMDevice.h"

class RCPWMDevice : public PWMDevice {
public:
    RCPWMDevice(uint8_t gpio_pin);

    const std::string getName() const override {
        return "RCPWM" + std::to_string(_gpio_pin);
    }
    const std::string getType() const override { return "RCPWM"; }
    const std::string getDetails() const override;

    bool setPercent(float percent);
private:
    float _millisecond_unit_factor;
    float _millisecond_unit;
};
