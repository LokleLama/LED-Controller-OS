#pragma once

#include <stdexcept>
#include <string>

#include "VariableStore/IVariable.h"
#include "Utils/ValueConverter.h"

class IntVariable : public IVariable {
public:
  // Constructor
  IntVariable(const std::string &name, int value, IntegerStringFormat format = IntegerStringFormat::DECIMAL) : IVariable(name), value_(value), format_(format) {}

  IntVariable(const IntVariable &other) : IVariable(other.getName()), value_(other.value_), format_(other.format_) {}
  IntVariable(IntVariable &&other) noexcept : IVariable(other.getName()), value_(std::move(other.value_)), format_(std::move(other.format_)) {}
  IntVariable &operator=(const IntVariable &other) {
    if (this != &other) {
      value_ = other.value_;
      format_ = other.format_;
    }
    return *this;
  }

  // Convert to string
  std::string asString() const override {
    return ValueConverter::toString(value_, format_);
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
    return set(ValueConverter::toInt(value, &format_));
  }
  bool set(float value) override {
    return set(static_cast<int>(value));
  }
  bool set(int value) override {
    if(!callCallback()) {
      return false;
    }
    value_ = value;
    return true;
  }
  bool setBool(bool value) override {
    return set(value ? 1 : 0);
  }

  // Get the type of the variable
  Type getType() const override { return Type::INT; }

private:
  int value_;
  IntegerStringFormat format_;
};
