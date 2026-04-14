#pragma once

#include "VariableStore/FloatVariable.h"

class CPUTemperature : public FloatVariable {
public:
  CPUTemperature(const std::string& name, float adc_voltage_reference = 3.3f, uint16_t max_adc_value = 4095)
   : FloatVariable(name, 0.0f) {
    _adc_voltage_per_tick = adc_voltage_reference / max_adc_value;
    setSystemVariable();
  }

  bool set(const std::string& value) override {
      return false;
  }

  bool set(float value) override {
    return false;
  }

  bool setBool(bool value) override {
    return false;
  }

  bool set(int value) override {
    float voltage = value * _adc_voltage_per_tick;
    float temperature = 27.0f - ((voltage - 0.706f) * 581.0575f);
    return FloatVariable::set(temperature);
  }

private:
  float _adc_voltage_per_tick;
};