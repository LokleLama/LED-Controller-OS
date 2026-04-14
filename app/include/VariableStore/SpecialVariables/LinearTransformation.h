#pragma once

#include "VariableStore/FloatVariable.h"

class LinearTransformation : public FloatVariable {
public:
  LinearTransformation(const std::string& name, float multiplier = 1.0f, float offset = 0.0f)
   : FloatVariable(name, 0.0f), _multiplier(multiplier), _offset(offset) {
    setSystemVariable();
  }

  bool set(const std::string &value) override {
    return set(std::stof(value));
  }

  bool set(float value) override {
    float transformed_value = value * _multiplier + _offset;
    return FloatVariable::set(transformed_value);
  }

  bool set(int value) override {
    return set(static_cast<float>(value));
  }

  bool setBool(bool value) override {
    return set(value ? 1.0f : 0.0f);
  }

private:
  float _multiplier;
  float _offset;
};