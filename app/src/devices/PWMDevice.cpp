#include "devices/PWMDevice.h"

#include "hardware/clocks.h"
#include "hardware/gpio.h"

#include <cmath>
#include <iostream>

PWMDevice::PWMDevice(uint8_t gpio_pin, int frequency_hz, bool phase_correct)
    : _gpio_pin(gpio_pin),
      _slice(0),
      _channel(0),
      _pwm_initialized(false),
      _frequency_hz(frequency_hz),
      _phase_correct(phase_correct),
      _wrap(0),
      _dma_channel(-1){
    if (_gpio_pin > 29) {
        _status = DeviceStatus::Error;
        return;
    }

    _slice = pwm_gpio_to_slice_num(_gpio_pin);
    _channel = pwm_gpio_to_channel(_gpio_pin);

    gpio_set_function(_gpio_pin, GPIO_FUNC_PWM);

    if (!configurePWM(frequency_hz, phase_correct)) {
        _status = DeviceStatus::Error;
        return;
    }

    setLevel(_wrap >> 1);
    pwm_set_enabled(_slice, true);
    _pwm_initialized = true;

    _status = DeviceStatus::Initialized;
}

PWMDevice::~PWMDevice() {
    if (_pwm_initialized) {
        pwm_set_enabled(_slice, false);
    }
    if (_dma_channel >= 0) {
        dma_channel_abort(_dma_channel);
        dma_channel_unclaim(_dma_channel);
        _dma_channel = -1;
    }
}

bool PWMDevice::configurePWM(int frequency_hz, bool phase_correct) {
    if (frequency_hz <= 8 || frequency_hz > 4000000) {
        return false;
    }

    int pwm_bits = 16;
    auto clk_hz = clock_get_hz(clk_sys);
    if(phase_correct){
        clk_hz >>= 1;
    }

    while (frequency_hz > (clk_hz >> pwm_bits)) {
        --pwm_bits;
        if (pwm_bits < 1) {
            return false;
        }
    }
    return configurePWM(frequency_hz, pwm_bits, phase_correct);
}

bool PWMDevice::configurePWM(int frequency_hz, int pwm_bits, bool phase_correct) {
    if (frequency_hz <= 8 || frequency_hz > 4000000) {
        return false;
    }

    auto clk_hz = clock_get_hz(clk_sys) >> pwm_bits;
    
    if(phase_correct){
        clk_hz >>= 1;
    }
    
    float clkdiv = static_cast<float>(clk_hz) / static_cast<float>(frequency_hz);
    if (clkdiv < 1.0f || clkdiv > 256.0f) {
        return false;
    }
    int int_clkdiv = static_cast<int>(clkdiv);
    int frac_clkdiv = static_cast<int>((clkdiv - int_clkdiv) * 16);
    
    pwm_set_clkdiv_int_frac(_slice, int_clkdiv, frac_clkdiv);
    pwm_set_wrap(_slice, (1 << pwm_bits) - 1);
    pwm_set_phase_correct(_slice, phase_correct);

    _wrap = (1 << pwm_bits) - 1;
    _pwm_bits = pwm_bits;

    pwm_set_chan_level(_slice, _channel, _wrap >> 1);

    float actual_divider = int_clkdiv + frac_clkdiv / 16.0f;
    _frequency_hz = static_cast<int>(clk_hz / actual_divider);
    
    return true;
}

bool PWMDevice::setLevel(uint16_t level) {
    if (_status == DeviceStatus::Error || _wrap == 0) {
        return false;
    }

    pwm_set_chan_level(_slice, _channel, level);
    return true;
}

bool PWMDevice::setScaledLevel(uint16_t level) {
    if (_status == DeviceStatus::Error || _wrap == 0) {
        return false;
    }

    pwm_set_chan_level(_slice, _channel, level >> (16 - _pwm_bits));
    return true;
}

bool PWMDevice::useDMA16(uint transfer_count) {
    if (transfer_count == 0) {
        return false;
    }
    if (_dma_channel >= 0) {
        return false;
    }

    _dma_channel = dma_claim_unused_channel(false);
    if (_dma_channel < 0) {
        return false;
    }

    dma_channel_config dc = dma_channel_get_default_config(_dma_channel);
    channel_config_set_transfer_data_size(&dc, DMA_SIZE_16);
    channel_config_set_read_increment(&dc, true);
    channel_config_set_write_increment(&dc, false);
    channel_config_set_dreq(&dc, pwm_get_dreq(_slice));

    volatile uint16_t* cc_reg = reinterpret_cast<volatile uint16_t*>(&pwm_hw->slice[_slice].cc) + _channel;
    dma_channel_configure(_dma_channel, &dc, const_cast<uint16_t*>(cc_reg), nullptr, transfer_count, false);

    return true;
}

bool PWMDevice::transfer(const uint16_t* data, size_t count) {
    if (!data || count == 0) {
        return false;
    }

    if ((_dma_channel < 0)) {
        for (size_t i = 0; i < count; ++i) {
            if (!setLevel(data[i])) {
                return false;
            }
        }
        return true;
    }

    if (dma_channel_is_busy(_dma_channel)) {
        return false;
    }

    dma_channel_transfer_from_buffer_now(_dma_channel, data, count);
    return true;
}

const std::string PWMDevice::getDetails() const {
    std::string details = "PWM" + std::to_string(_slice) + "." + std::to_string(_channel) + " on GPIO" + std::to_string(_gpio_pin) + "\n";
    details += "Frequency: " + std::to_string(_frequency_hz) + "HZ";
    details += _phase_correct ? " (phase-correct)\n" : "\n";

    details += "Wrap: " + std::to_string(_wrap) + "\n";

    if (_dma_channel < 0) {
        details += "DMA not used\n";
    } else {
        details += "DMA CH " + std::to_string(_dma_channel) + "\n";
    }

    return details;
}
