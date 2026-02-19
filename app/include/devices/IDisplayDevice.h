#pragma once

#include "devices/IDevice.h"

class IDisplayDevice : public IDevice, public IDeviceUser {
public:
    virtual void setValue(const std::string& value) = 0;
};