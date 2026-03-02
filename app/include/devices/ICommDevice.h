#pragma once

#include "devices/IDevice.h"

#include <string>
#include <vector>

class ICommDevice : public IDevice {
public:
    virtual int send(const std::vector<uint8_t>& data){
        return send(data.data(), data.size());
    }
    virtual int send(const uint8_t* data, size_t length) = 0;

    virtual int dataAvailable() = 0;
    virtual int receive(uint8_t* buffer, size_t length) = 0;
};