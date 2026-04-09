#pragma once

#include "hardware/adc.h"
#include "devices/IDevice.h"
#include "VariableStore/IVariable.h"
#include "Mainloop.h"

class ADCDevice : public ICreateSharedFromThis<ADCDevice>, public IDevice {
public:
    ADCDevice(const std::string& name, int adc_channel, std::shared_ptr<IVariable> value_variable, int sampling_intervall = 100);

    const std::string getName() const override { return _name; }
    const std::string getType() const override { return "ADC"; }
    const std::string getDetails() const override;

    uint16_t readValue() const;

private:
    std::string _name;
    std::shared_ptr<IVariable> _valueVariable;
    int _samplingIntervall;
    Mainloop::TaskHandle _readoutTask;

    uint8_t _adc_channel;
    int8_t _adc_pins[5] = {26, 27, 28, 29, -1};
    static bool _adc_initialized[5];
    
    bool ExecuteTask();
};
