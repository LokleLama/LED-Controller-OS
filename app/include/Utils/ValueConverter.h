#pragma once

#include <string>
#include <cstdint>

enum class IntegerStringFormat { UNKNOWN, BINARY, OCTAL, DECIMAL, HEX, HEX_COLOR};

class ValueConverter {
public:
    static unsigned int toUInt(const std::string& str, IntegerStringFormat* format){
        return static_cast<unsigned int>(toInt(str, format));
    }
    static unsigned int toUInt(const std::string& str){
        IntegerStringFormat format;
        return toUInt(str, &format);
    }
    static int toInt(const std::string& str, IntegerStringFormat* format);
    static int toInt(const std::string& str){
        IntegerStringFormat format;
        return toInt(str, &format);
    }

    static std::string toString(int value, IntegerStringFormat format = IntegerStringFormat::DECIMAL);
};