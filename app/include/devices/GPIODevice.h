#pragma once

#include "devices/IDevice.h"
#include "hardware/gpio.h"

#include <vector>
#include <memory>
#include <string>
#include <cstdint>

class GPIODevice : public ICreateSharedFromThis<GPIODevice>, public IDevice {
public:
    enum class GPIOConfiguration {
        INPUT,
        OUTPUT,
        INPUT_PULLUP,
        INPUT_PULLDOWN,
        OUTPUT_OPEN_DRAIN,
        OUTPUT_OPEN_SOURCE,
        INVALID
    };

    static GPIOConfiguration parseConfiguration(const std::string& config_str);
    static std::string configurationToString(GPIOConfiguration config);

    GPIODevice(int gpio_number, GPIOConfiguration config = GPIOConfiguration::INPUT);

    const std::string getName() const override { return "GPIO" + std::to_string(_gpio_number); }
    const std::string getType() const override { return "GPIO"; }
    const std::string getDetails() const override;

    void set(bool state);
    bool get() const { return gpio_get(_gpio_number); }

    void setConfiguration(GPIOConfiguration config);
    bool isOutput() const { return gpio_is_dir_out(_gpio_number); }

private:
    int _gpio_number;
    GPIOConfiguration _configuration;

    static bool _irq_initialized;
};
