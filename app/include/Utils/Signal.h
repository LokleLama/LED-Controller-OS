#pragma once

#include <string>
#include <cstdint>
#include <cctype>

#include "Utils/ValueConverter.h"

using Signal = uint32_t;

class SignalConverter {
public:
    static Signal fromString(const std::string& str) {
        if(str.size() != 4) {
            return ValueConverter::toInt(str);
        }
        return (static_cast<Signal>(str[0]) << 24) | (static_cast<Signal>(str[1]) << 16) | (static_cast<Signal>(str[2]) << 8) | static_cast<Signal>(str[3]);
    }

    static std::string toString(Signal signal) {
        char chars[5] = {0};
        chars[0] = (signal >> 24) & 0xFF;
        chars[1] = (signal >> 16) & 0xFF;
        chars[2] = (signal >> 8) & 0xFF;
        chars[3] = signal & 0xFF;

        for(int i = 0; i < 4; i++) {
            if(!std::isprint(chars[i])) {
                return ValueConverter::toString(signal, IntegerStringFormat::HEX);
            }
        }

        return std::string(chars);
    }
};