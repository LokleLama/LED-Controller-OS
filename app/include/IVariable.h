#pragma once

#include <stdexcept>
#include <string>
#include <variant>

class IVariable {
public:
  // Supported types
  using VariableType = std::variant<float, bool, int, std::string>;

  // Destructor
  virtual ~IVariable() = default;

  // Convert to string
  virtual std::string toString() const = 0;

  // Convert to float
  virtual float toFloat() const = 0;

  // Convert to int
  virtual int toInt() const = 0;

  // Convert to bool
  virtual bool toBool() const = 0;

  // Set from string
  virtual void fromString(const std::string &value) = 0;

  // Get the underlying value
  virtual const VariableType &getValue() const = 0;

  // Set the value
  virtual void setValue(const VariableType &value) = 0;
};
