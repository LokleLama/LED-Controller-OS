#pragma once

#include <stdexcept>
#include <string>
#include <variant>

class IVariable {
public:
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
};
