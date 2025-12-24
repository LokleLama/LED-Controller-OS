#pragma once

#include <stdexcept>
#include <string>
#include <variant>

#include "VariableStore/IVariable.h"

class IntVariable : public IVariable {
public:
  // Constructor
  IntVariable(int value) : value_(value) {}

  IntVariable(const IntVariable &other) : value_(other.value_) {}
  IntVariable(IntVariable &&other) noexcept : value_(std::move(other.value_)) {}
  IntVariable &operator=(const IntVariable &other) {
    if (this != &other) {
      value_ = other.value_;
    }
    return *this;
  }

  // Convert to string
  std::string asString() const override { return std::to_string(value_); }

  // Convert to float
  float asFloat() const override { return static_cast<float>(value_); };

  // Convert to int
  int asInt() const override { return value_; };

  // Convert to bool
  bool asBool() const override { return value_ != 0; }

  // Set the value
  bool set(const std::string &value) override {
    value_ = std::stoi(value, nullptr, 10);
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
};
