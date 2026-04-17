#include "devices/GPIODevice.h"
#include "Utils/Signal.h"
#include "Mainloop.h"

bool GPIODevice::_irq_initialized = false;

GPIODevice::GPIODevice(int gpio_number, GPIODevice::GPIOConfiguration config)
    : _gpio_number(gpio_number) {
    gpio_init(_gpio_number);
    setConfiguration(config);

    if(!_irq_initialized) {
        gpio_set_irq_callback([](uint gpio, uint32_t events) {
            Signal sig = 0x67703030;

            uint tens = (gpio * 0x19999A) >> 24; // Approximate division by 10
            uint units = gpio - (tens * 10);

            sig += (tens << 8) | units;
            
            Mainloop::getInstance().triggerSignal(sig);
        });
        _irq_initialized = true;
    }

    gpio_set_irq_enabled(_gpio_number, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

    _status = DeviceStatus::Initialized;
}

const std::string GPIODevice::getDetails() const {
    return "GPIO" + std::to_string(_gpio_number) + " = " + (get() ? "HIGH" : "LOW") + ", Direction: " + (isOutput() ? "OUTPUT" : "INPUT");
}

void GPIODevice::set(bool state) {
    if(_configuration == GPIOConfiguration::OUTPUT_OPEN_DRAIN) {
        gpio_set_dir(_gpio_number, state ? GPIO_IN : GPIO_OUT);
    }else if(_configuration == GPIOConfiguration::OUTPUT_OPEN_SOURCE) {
        gpio_set_dir(_gpio_number, state ? GPIO_OUT : GPIO_IN);
    } else {
        gpio_put(_gpio_number, state);
    }
}

void GPIODevice::setConfiguration(GPIODevice::GPIOConfiguration config) {
    switch (config) {
        case GPIODevice::GPIOConfiguration::INPUT:
            gpio_set_dir(_gpio_number, GPIO_IN);
            break;
        case GPIODevice::GPIOConfiguration::OUTPUT:
            gpio_set_dir(_gpio_number, GPIO_OUT);
            break;
        case GPIODevice::GPIOConfiguration::INPUT_PULLUP:
            gpio_set_dir(_gpio_number, GPIO_IN);
            gpio_pull_up(_gpio_number);
            gpio_put(_gpio_number, false);
            break;
        case GPIODevice::GPIOConfiguration::INPUT_PULLDOWN:
            gpio_set_dir(_gpio_number, GPIO_IN);
            gpio_pull_down(_gpio_number);
            gpio_put(_gpio_number, true);
            break;
        case GPIODevice::GPIOConfiguration::OUTPUT_OPEN_DRAIN:
            gpio_set_dir(_gpio_number, GPIO_IN);
            gpio_put(_gpio_number, false);
            break;
        case GPIODevice::GPIOConfiguration::OUTPUT_OPEN_SOURCE:
            gpio_set_dir(_gpio_number, GPIO_IN);
            gpio_put(_gpio_number, true);
            break;
    }
    _configuration = config;
}

GPIODevice::GPIOConfiguration GPIODevice::parseConfiguration(const std::string& config_str) {
    if (config_str == "INPUT" || config_str == "in") {
        return GPIOConfiguration::INPUT;
    } else if (config_str == "OUTPUT" || config_str == "out") {
        return GPIOConfiguration::OUTPUT;
    } else if (config_str == "INPUT_PULLUP" || config_str == "pullup") {
        return GPIOConfiguration::INPUT_PULLUP;
    } else if (config_str == "INPUT_PULLDOWN" || config_str == "pulldown") {
        return GPIOConfiguration::INPUT_PULLDOWN;
    } else if (config_str == "OUTPUT_OPEN_DRAIN" || config_str == "open_drain") {
        return GPIOConfiguration::OUTPUT_OPEN_DRAIN;
    } else if (config_str == "OUTPUT_OPEN_SOURCE" || config_str == "open_source") {
        return GPIOConfiguration::OUTPUT_OPEN_SOURCE;
    }
    return GPIOConfiguration::INVALID;
}

std::string GPIODevice::configurationToString(GPIODevice::GPIOConfiguration config) {
    switch (config) {
        case GPIOConfiguration::INPUT: return "in";
        case GPIOConfiguration::OUTPUT: return "out";
        case GPIOConfiguration::INPUT_PULLUP: return "pullup";
        case GPIOConfiguration::INPUT_PULLDOWN: return "pulldown";
        case GPIOConfiguration::OUTPUT_OPEN_DRAIN: return "open_drain";
        case GPIOConfiguration::OUTPUT_OPEN_SOURCE: return "open_source";
        default: return "INVALID";
    }
}
