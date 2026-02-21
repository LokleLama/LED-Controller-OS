#include "Utils/ValueConverter.h"
#include <iomanip>
#include <sstream>

int ValueConverter::toInt(const std::string& str, IntegerStringFormat* format) {
    *format = IntegerStringFormat::DECIMAL;
    if(str[0] == '0' && str.size() > 1){
      if(str[1] == 'b' || str[1] == 'B'){
        *format = IntegerStringFormat::BINARY;
      } else if (str[1] == 'x' || str[1] == 'X'){
        *format = IntegerStringFormat::HEX;
      } else if (str[1] == 'o' || str[1] == 'O'){
        *format = IntegerStringFormat::OCTAL;
      }
    }
    switch(*format){
      case IntegerStringFormat::BINARY:
        return std::stoi(str.substr(2), nullptr, 2);
      case IntegerStringFormat::OCTAL:
        return std::stoi(str.substr(2), nullptr, 8);
      case IntegerStringFormat::HEX:
        return std::stoi(str.substr(2), nullptr, 16);
      case IntegerStringFormat::DECIMAL:
      default:
        return std::stoi(str, nullptr, 10);
    }
}

std::string ValueConverter::toString(int value, IntegerStringFormat format) {
    switch (format) {
      case IntegerStringFormat::BINARY: {
        std::string result = "0b";
        int val = value;
        for (int n = 0; n < sizeof(int); n++) {
          for (int i = 0; i < 8; i++) {
            result += (val & (1 << (sizeof(int) * 8 - 1))) ? '1' : '0';
            val <<= 1;
          }
          if(n < sizeof(int) - 1) {
            result += '_';
          }
        }
        return result;
      }
      case IntegerStringFormat::OCTAL: {
        std::ostringstream oss;
        oss << "0o" << std::oct << value;
        return oss.str();
      }
      case IntegerStringFormat::HEX: {
        std::ostringstream oss;
        oss << "0x" << std::hex << value;
        return oss.str();
      }
      case IntegerStringFormat::DECIMAL:
      default:
        break;
    }
    return std::to_string(value);
}
