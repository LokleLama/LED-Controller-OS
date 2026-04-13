#pragma once

#include <stdexcept>
#include <string>
#include <variant>
#include <memory>

#include "VariableStore/IVariable.h"
#include "devices/GPIODevice.h"

class GPIOVariable : public IVariable {
public:
  // Constructor
  GPIOVariable(const std::string &name, std::shared_ptr<GPIODevice> device) : IVariable(name), _device(device) {
    setSystemVariable();
  }

  GPIOVariable(const GPIOVariable &other) : IVariable(other.getName()), _device(other._device) {}
  GPIOVariable(GPIOVariable &&other) noexcept
      : IVariable(other.getName()), _device(std::move(other._device)) {}
      
  GPIOVariable &operator=(const GPIOVariable &other) {
    if (this != &other) {
      _device = other._device;
    }
    return *this;
  }

  // Convert to string
  std::string asString() const override { return _device->get() ? "1" : "0"; }

  // Convert to float
  float asFloat() const override { return _device->get() ? 1.0f : 0.0f; };

  // Convert to int
  int asInt() const override { return _device->get() ? 1 : 0; };

  // Convert to bool
  bool asBool() const override { return _device->get(); }

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
    _device->set(value);
    return true;
  }

  // Get the type of the variable
  Type getType() const override { return Type::BOOL; }

private:
    std::shared_ptr<GPIODevice> _device;
};
