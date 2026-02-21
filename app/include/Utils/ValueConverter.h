#pragma once

#include <string>
#include <cstdint>

enum class IntegerStringFormat { UNKNOWN, BINARY, OCTAL, DECIMAL, HEX };

class ValueConverter {
public:
    static int toInt(const std::string& str, IntegerStringFormat* format);
    static int toInt(const std::string& str){
        IntegerStringFormat format;
        return toInt(str, &format);
    }

    static std::string toString(int value, IntegerStringFormat format = IntegerStringFormat::DECIMAL);
};