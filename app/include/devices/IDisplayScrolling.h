#pragma once

#include <string>
#include <cstdint>

class IDisplayScrolling {
public:
    IDisplayScrolling() = default;

    enum class ScrollingDirection {
        STOP,
        LEFT,
        RIGHT,
        UP,       // Not yet implemented
        DOWN      // Not yet implemented
    };

    virtual void setScrollingSpeed(int speed) = 0;
    virtual void setScrollingDirection(ScrollingDirection direction) = 0;
};
