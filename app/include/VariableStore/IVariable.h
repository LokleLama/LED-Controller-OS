#pragma once

#include <string>
#include <functional>

class IVariable {
public:
  using SetCallback = std::function<bool(const std::string& key)>;

  enum class Type { STRING, INT, FLOAT, BOOL };

  IVariable(const std::string &name) : _name(name) {}

  // Destructor
  virtual ~IVariable() = default;

  // Convert to string
  virtual std::string asString() const = 0;
  virtual std::string asValueString() const { return asString(); }

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
  virtual bool setBool(bool value) = 0;

  // Get the type of the variable
  virtual Type getType() const = 0;

  const std::string& getName() const { return _name; }

  void setSystemVariable() {
    _isSystemVariable = true;
  }
  bool isSystemVariable() const {
    return _isSystemVariable;
  }

  void setCallback(SetCallback callback) { _callback = callback; }

protected:
  bool callCallback() {
    if (_callback) {
      return _callback(_name);
    }
    return true;
  }

private:
  std::string _name;
  SetCallback _callback = nullptr;
  bool _isSystemVariable = false;
};
