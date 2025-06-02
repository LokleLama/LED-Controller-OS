#pragma once

#include <stdexcept>
#include <string>
#include <variant>

#include "IVariable.h"

class FloatVariable : public IVariable {
public:
  // Constructor
  FloatVariable(float value) : value_(value) {}

  FloatVariable(const FloatVariable &other) : value_(other.value_) {}
  FloatVariable(FloatVariable &&other) noexcept
      : value_(std::move(other.value_)) {}
  FloatVariable &operator=(const FloatVariable &other) {
    if (this != &other) {
      value_ = other.value_;
    }
    return *this;
  }

  // Convert to string
  std::string asString() const override { return std::to_string(value_); }

  // Convert to float
  float asFloat() const override { return value_; };

  // Convert to int
  int asInt() const override { return static_cast<int>(value_); };

  // Convert to bool
  bool asBool() const override {
    if (value_ == 0.0f) {
      return false;
    }
    return true;
  }

  // Set the value
  bool set(const std::string &value) override {
    value_ = std::stof(value);
    return true;
  }
  bool set(float value) override {
    value_ = value;
    return true;
  }
  bool set(int value) override {
    value_ = static_cast<float>(value);
    return true;
  }
  bool set(bool value) override {
    value_ = value ? 1.0f : 0.0f;
    return true;
  }

private:
  float value_;
};
