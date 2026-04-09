#pragma once

#include <stdexcept>
#include <string>
#include <variant>

#include "VariableStore/IVariable.h"

class BoolVariable : public IVariable {
public:
  // Constructor
  BoolVariable(const std::string &name, bool value) : IVariable(name), value_(value) {}

  BoolVariable(const BoolVariable &other) : IVariable(other.getName()), value_(other.value_) {}
  BoolVariable(BoolVariable &&other) noexcept
      : IVariable(other.getName()), value_(std::move(other.value_)) {}
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
      return setBool(true);
    } else if (value == "false" || value == "0") {
      return setBool(false);
    }
    return false;
  }
  bool set(float value) override {
    return setBool(value != 0.0f);
  }
  bool set(int value) override {
    return setBool(value != 0);
  }
  bool setBool(bool value) override {
    if(!callCallback()) {
      return false;
    }
    value_ = value;
    return true;
  }

  // Get the type of the variable
  Type getType() const override { return Type::BOOL; }

private:
  bool value_;
};
