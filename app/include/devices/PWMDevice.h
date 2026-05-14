#pragma once

#include "devices/IDevice.h"

#include "hardware/pwm.h"
#include "hardware/dma.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class PWMDevice : public ICreateSharedFromThis<PWMDevice>, public IDevice {
public:
    PWMDevice(uint8_t gpio_pin, int frequency_hz, bool phase_correct = false);
    ~PWMDevice();

    const std::string getName() const override {
        return "PWM" + std::to_string(_gpio_pin);
    }
    const std::string getType() const override { return "PWM"; }
    const std::string getDetails() const override;

    bool setLevel(uint16_t level);
    bool setScaledLevel(uint16_t level);

    bool configurePWM(int frequency_hz, bool phase_correct = true);
    bool configurePWM(int frequency_hz, int pwm_bits, bool phase_correct = true);

    int getFrequency() const { return _frequency_hz; }

    bool useDMA16(uint transfer_count);

    bool transfer(const std::vector<uint16_t>& data) { return transfer(data.data(), data.size()); }
    bool transfer(const uint16_t* data, size_t count);

    uint8_t getPin() const { return _gpio_pin; }
    uint8_t getSlice() const { return _slice; }
    uint8_t getChannel() const { return _channel; }

    uint16_t getWrap() const { return _wrap; }

private:
    uint8_t _gpio_pin;
    uint8_t _slice;
    uint8_t _channel;
    uint8_t _pwm_bits;
    bool _pwm_initialized;

    int _frequency_hz;
    bool _phase_correct;
    uint16_t _wrap;

    int _dma_channel;
    

    bool configurePWM(float cycle_time_us, bool phase_correct);
    bool scaleAndExpandSamples(const uint16_t* data, size_t count);
};
