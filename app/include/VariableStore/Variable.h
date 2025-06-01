#pragma once

#include <stdexcept>
#include <string>
#include <variant>

#include "IVariable.h"

class Variable : public IVariable {
public:
  // Constructor
  Variable(const IVariable::VariableType &value) : value_(value) {}

  Variable(const Variable &other) : value_(other.value_) {}
  Variable(Variable &&other) noexcept : value_(std::move(other.value_)) {}
  Variable &operator=(const Variable &other) {
    if (this != &other) {
      value_ = other.value_;
    }
    return *this;
  }

  // Convert to string
  std::string toString() const override;

  // Convert to float
  float toFloat() const override;

  // Convert to int
  int toInt() const override;

  // Convert to bool
  bool toBool() const override;

  // Set from string
  void fromString(const std::string &value) override;

  // Get the underlying value
  const IVariable::VariableType &getValue() const override { return value_; }

  // Set the value
  void setValue(const IVariable::VariableType &value) override {
    value_ = value;
  }

private:
  IVariable::VariableType value_;
};
