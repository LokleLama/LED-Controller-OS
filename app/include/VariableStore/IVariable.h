#pragma once

#include <stdexcept>
#include <string>
#include <variant>

class IVariable {
public:
  enum class Type { STRING, INT, FLOAT, BOOL };

  // Destructor
  virtual ~IVariable() = default;

  // Convert to string
  virtual std::string asString() const = 0;

  // Convert to float
  virtual float asFloat() const = 0;

  // Convert to int
  virtual int asInt() const = 0;

  // Convert to bool
  virtual bool asBool() const = 0;

  // Set the value
  virtual bool set(const std::string &value) = 0;
  virtual bool set(float value) = 0;
  virtual bool set(int value) = 0;
  virtual bool set(bool value) = 0;

  // Get the type of the variable
  virtual Type getType() const = 0;
};
