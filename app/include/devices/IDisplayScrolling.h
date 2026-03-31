#pragma once

#include <string>
#include <cstdint>

class IDisplayScrolling {
public:
    IDisplayScrolling() = default;

    virtual void setScrollingSpeed(int speed) = 0;
};
