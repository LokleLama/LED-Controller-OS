#pragma once

#include <stdexcept>
#include <string>
#include <variant>

#include "VariableStore/IVariable.h"

class BoolVariable : public IVariable {
public:
  // Constructor
  BoolVariable(bool value) : value_(value) {}

  BoolVariable(const BoolVariable &other) : value_(other.value_) {}
  BoolVariable(BoolVariable &&other) noexcept
      : value_(std::move(other.value_)) {}
  BoolVariable &operator=(const BoolVariable &other) {
    if (this != &other) {
      value_ = other.value_;
    }
    return *this;
  }

  // Convert to string
  std::string asString() const override { return value_ ? "true" : "false"; }

  // Convert to float
  float asFloat() const override { return value_ ? 1.0f : 0.0f; };

  // Convert to int
  int asInt() const override { return value_ ? 1 : 0; };

  // Convert to bool
  bool asBool() const override { return value_; }

  // Set the value
  bool set(const std::string &value) override {
    if (value == "true" || value == "1") {
      value_ = true;
    } else if (value == "false" || value == "0") {
      value_ = false;
    } else {
      return false;
    }
    return true;
  }
  bool set(float value) override {
    value_ = (value != 0.0f);
    return true;
  }
  bool set(int value) override {
    value_ = (value != 0);
    return true;
  }
  bool set(bool value) override {
    value_ = value;
    return true;
  }

  // Get the type of the variable
  Type getType() const override { return Type::BOOL; }

private:
  bool value_;
};
