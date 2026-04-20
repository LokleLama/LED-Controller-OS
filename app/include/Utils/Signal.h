#pragma once

#include <string>
#include <cstdint>
#include <cctype>

#include "Utils/ValueConverter.h"

using Signal = uint32_t;

struct SignalFilter {
    Signal signal;
    uint32_t mask;
};

class SignalConverter {
public:
    static SignalFilter fromString(const std::string& str) {
        if(str.empty()) {
            return {0, 0xFFFFFFFF};
        }
        if(str.size() <= 4) {
            if(str.size() < 4) {
                int padding = 4 - str.size();
                char chars[5] = {0};
                int n = 0;
                for(int i = 0; i < str.size(); i++) {
                    if(str[i] == '?' || str[i] == '*') {
                        for(int j = 0; j < padding; j++) {
                            chars[n++] = '*';
                        }
                        chars[n++] = '*';
                    } else {
                        chars[n++] = str[i];
                    }
                }
                return fromString(chars);
            }
            Signal sig = 0;
            uint32_t mask = 0;
            for(int i = 0; i < str.size(); i++) {
                sig <<= 8;
                sig |= static_cast<Signal>(str[i]);
                mask <<= 8;
                mask |= 0xFF;
                if(str[i] == '?' || str[i] == '*') {
                    mask &= 0xFFFFFF00;
                }
            }
            return {sig, mask};
        }
        return {ValueConverter::toUInt(str), 0xFFFFFFFF};
    }

    static std::string toString(Signal signal) {
        char chars[5] = {0};
        chars[0] = (signal >> 24) & 0xFF;
        chars[1] = (signal >> 16) & 0xFF;
        chars[2] = (signal >> 8) & 0xFF;
        chars[3] = signal & 0xFF;

        for(int i = 0; i < 4; i++) {
            if(!std::isgraph(chars[i])) {
                return ValueConverter::toString(signal, IntegerStringFormat::HEX);
            }
        }

        return std::string(chars);
    }

    static std::string toString(SignalFilter signal) {
        Signal maskedSignal = signal.signal & signal.mask;
        maskedSignal |= ((~signal.mask) & 0x2A2A2A2A); // Replace masked bits with '*'
        return toString(maskedSignal);
    }
};