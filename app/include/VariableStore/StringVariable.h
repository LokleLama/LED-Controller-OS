#pragma once

#include <stdexcept>
#include <string>
#include <variant>

#include "VariableStore/IVariable.h"

class StringVariable : public IVariable {
public:
  // Constructor
  StringVariable(const std::string &value) : value_(value) {}

  StringVariable(const StringVariable &other) : value_(other.value_) {}
  StringVariable(StringVariable &&other) noexcept
      : value_(std::move(other.value_)) {}
  StringVariable &operator=(const StringVariable &other) {
    if (this != &other) {
      value_ = other.value_;
    }
    return *this;
  }

  // Convert to string
  std::string asString() const override { return value_; }

  // Convert to float
  float asFloat() const override { return std::stof(value_); };

  // Convert to int
  int asInt() const override { return std::stoi(value_); };

  // Convert to bool
  bool asBool() const override {
    if (value_ == "true" || value_ == "1") {
      return true;
    }
    return false;
  }

  // Set the value
  bool set(const std::string &value) override {
    value_ = value;
    return true;
  }
  bool set(float value) override {
    value_ = std::to_string(value);
    return true;
  }
  bool set(int value) override {
    value_ = std::to_string(value);
    return true;
  }
  bool set(bool value) override {
    value_ = std::to_string(value);
    return true;
  }

  // Get the type of the variable
  Type getType() const override { return Type::STRING; }

private:
  std::string value_;
};
