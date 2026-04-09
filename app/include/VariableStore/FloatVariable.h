#pragma once

#include <stdexcept>
#include <string>
#include <variant>

#include "VariableStore/IVariable.h"

class FloatVariable : public IVariable {
public:
  // Constructor
  FloatVariable(const std::string &name, float value) : IVariable(name), value_(value) {}

  FloatVariable(const FloatVariable &other) : IVariable(other.getName()), value_(other.value_) {}
  FloatVariable(FloatVariable &&other) noexcept
      : IVariable(other.getName()), value_(std::move(other.value_)) {}
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
    return set(std::stof(value));
  }
  bool set(float value) override {
    if(!callCallback()) {
      return false;
    }
    value_ = value;
    return true;
  }
  bool set(int value) override {
    return set(static_cast<float>(value));
  }
  bool setBool(bool value) override {
    return set(value ? 1.0f : 0.0f);
  }

  // Get the type of the variable
  Type getType() const override { return Type::FLOAT; }

private:
  float value_;
};
