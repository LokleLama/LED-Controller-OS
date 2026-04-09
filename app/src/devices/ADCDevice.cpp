#include "devices/ADCDevice.h"

#include <iostream>

bool ADCDevice::_adc_initialized[5] = {false, false, false, false, false};

ADCDevice::ADCDevice(const std::string& name, int adc_channel, std::shared_ptr<IVariable> value_variable, int sampling_intervall)
    : _name(name), _valueVariable(value_variable), _samplingIntervall(sampling_intervall) {

    if(adc_channel < 0 || adc_channel > 4) {
        std::cerr << "Invalid ADC channel: " << adc_channel << ". Must be between 0 and 4." << std::endl;
        
        _status = DeviceStatus::Error;
        return;
    }
    if(_adc_initialized[adc_channel]) {
        std::cerr << "ADC channel " << adc_channel << " is already initialized." << std::endl;
        
        _status = DeviceStatus::Error;
        return;
    }
    _adc_channel = adc_channel;

    bool adc_is_initialized = false;
    for(int i = 0; i < 5; i++) {
        adc_is_initialized |= _adc_initialized[i];
    }

    if (!adc_is_initialized) {
        adc_init();
    }
    if(adc_channel == 4) {
        adc_set_temp_sensor_enabled(true);
    }

    if(_adc_pins[adc_channel] >= 0) {
        adc_gpio_init(_adc_pins[adc_channel]);
    }

    if(sampling_intervall > 0) {
        _readoutTask = Mainloop::getInstance().registerTimedTask(name + ".Readout", [this]() { return ExecuteTask(); }, _samplingIntervall);
    }
    
    _adc_initialized[adc_channel] = true;
    _status = DeviceStatus::Initialized;
}

const std::string ADCDevice::getDetails() const {
    return "ADC device on channel " + std::to_string(_adc_channel) + " with sampling interval " + std::to_string(_samplingIntervall) + "ms";
}

uint16_t ADCDevice::readValue() const {
    adc_select_input(_adc_channel);
    return adc_read();
}

bool ADCDevice::ExecuteTask() {
    uint16_t value = readValue();
    if(_valueVariable) {
        _valueVariable->set(value);
        return true;
    }
    return false;
}