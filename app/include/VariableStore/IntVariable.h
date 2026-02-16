#pragma once

#include <stdexcept>
#include <string>
#include <variant>
#include <iostream>
#include <iomanip>
#include <sstream>

#include "VariableStore/IVariable.h"

class IntVariable : public IVariable {
public:
  enum class Format { BINARY, OCTAL, DECIMAL, HEX, };

public:
  // Constructor
  IntVariable(int value, Format format = Format::DECIMAL) : value_(value), format_(format) {}

  IntVariable(const IntVariable &other) : value_(other.value_), format_(other.format_) {}
  IntVariable(IntVariable &&other) noexcept : value_(std::move(other.value_)), format_(std::move(other.format_)) {}
  IntVariable &operator=(const IntVariable &other) {
    if (this != &other) {
      value_ = other.value_;
      format_ = other.format_;
    }
    return *this;
  }

  // Convert to string
  std::string asString() const override {
    switch (format_) {
      case Format::BINARY: {
        std::string result = "0b";
        int val = value_;
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
      case Format::OCTAL: {
        std::ostringstream oss;
        oss << "0o" << std::oct << value_;
        return oss.str();
      }
      case Format::HEX: {
        std::ostringstream oss;
        oss << "0x" << std::hex << value_;
        return oss.str();
      }
      case Format::DECIMAL:
      default:
        break;
    }
    return std::to_string(value_);
  }

  std::string asValueString() const override {
    return std::to_string(value_);
  }

  // Convert to float
  float asFloat() const override { return static_cast<float>(value_); };

  // Convert to int
  int asInt() const override { return value_; };

  // Convert to bool
  bool asBool() const override { return value_ != 0; }

  // Set the value
  bool set(const std::string &value) override {
    format_ = Format::DECIMAL;
    if(value[0] == '0' && value.size() > 1){
      if(value[1] == 'b' || value[1] == 'B'){
        format_ = Format::BINARY;
      } else if (value[1] == 'x' || value[1] == 'X'){
        format_ = Format::HEX;
      } else if (value[1] == 'o' || value[1] == 'O'){
        format_ = Format::OCTAL;
      }
    }
    switch(format_){
      case Format::BINARY:
        value_ = std::stoi(value.substr(2), nullptr, 2);
        break;
      case Format::OCTAL:
        value_ = std::stoi(value.substr(2), nullptr, 8);
        break;
      case Format::HEX:
        value_ = std::stoi(value.substr(2), nullptr, 16);
        break;
      case Format::DECIMAL:
      default:
        value_ = std::stoi(value, nullptr, 10);
        break;
    }

    return true;
  }
  bool set(float value) override {
    value_ = static_cast<int>(value);
    return true;
  }
  bool set(int value) override {
    value_ = value;
    return true;
  }
  bool set(bool value) override {
    value_ = value ? 1 : 0;
    return true;
  }

  // Get the type of the variable
  Type getType() const override { return Type::INT; }

private:
  int value_;
  Format format_;
};
