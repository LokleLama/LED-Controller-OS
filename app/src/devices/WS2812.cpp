#include "devices/WS2812.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

#include "hardware/clocks.h"
#include "hardware/pio.h"

#include "PIO/led.pio.h"
#include <cstring>
#include <iostream>
#include <vector>

int WS2812Device::_program_offset_pio[2] = {-1, -1};

WS2812Device::WS2812Device(std::shared_ptr<PIODevice> pio, uint pin, uint num_leds, uint bits_per_pixel,
         float freq, const std::string& name)
    : _pio(pio), _pin(pin), _num_leds(num_leds),
      _bits_per_pixel(bits_per_pixel), _name(name) {

  // Load the PIO program into the PIO memory
  uint offset = 0;

  if (_program_offset_pio[_pio->getPIONumber()] == -1) {
    _program_offset_pio[_pio->getPIONumber()] = _pio->addProgram(&led_program);
  }else{
    _pio->setProgramOffset(_program_offset_pio[_pio->getPIONumber()]);
  }
  offset = _program_offset_pio[_pio->getPIONumber()];
  if (_num_leds > DMA_THRESHOLD) {
    _pio->useDMA(_num_leds * (_bits_per_pixel / 8));
  }

  led_program_init(_pio->getPIO(), _pio->getSM(), offset, _pin, freq, _bits_per_pixel);

  _status = DeviceStatus::Initialized;
}

const std::string WS2812Device::getDetails() const {
  return "WS2812 LED strip on pin " + std::to_string(_pin) + 
         " with " + std::to_string(_num_leds) + " LEDs (" + 
         std::to_string(_bits_per_pixel) + " bits/pixel)";
}

bool WS2812Device::setPattern(const uint32_t* data, size_t count) {
  if (count < _num_leds) {
    return false; 
  }
  return _pio->transfer(data, _num_leds);
}
