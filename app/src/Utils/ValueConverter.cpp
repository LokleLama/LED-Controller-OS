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
    if(str[0] == '#' && str.size() > 1){
      *format = IntegerStringFormat::HEX;
    }
    long long result = 0;
    switch(*format){
      case IntegerStringFormat::BINARY:
        result = std::stoll(str.substr(2), nullptr, 2);
        break;
      case IntegerStringFormat::OCTAL:
        result = std::stoll(str.substr(2), nullptr, 8);
        break;
      case IntegerStringFormat::HEX:
        if(str[0] == '#'){
          result = std::stoll(str.substr(1), nullptr, 16);
        } else {
          result = std::stoll(str.substr(2), nullptr, 16);
        }
        break;
      case IntegerStringFormat::DECIMAL:
      default:
        result = std::stoll(str, nullptr, 10);
        break;
    }
    return static_cast<int>(result);
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
