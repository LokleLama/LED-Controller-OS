#pragma once

#include "devices/IDevice.h"

#include <string>
#include <cstdint>

class IDisplayDevice : public IDevice {
public:
    IDisplayDevice() = default;
    IDisplayDevice(uint32_t color) : _color(color) {}

    virtual void setValue(const std::string& value) = 0;
    virtual void setColor(uint32_t color){
        _color = color;
    }

protected:
    uint32_t _color = 0x03030303;
};