#include "devices/WS2812.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

#include "hardware/clocks.h"
#include "hardware/pio.h"

#include "PIO/led.pio.h"
#include <cstring>
#include <iostream>
#include <vector>

int WS2812::_program_offset_pio[2] = {-1, -1};

WS2812::WS2812(std::shared_ptr<PIODevice> pio, uint pin, uint num_leds, uint bits_per_pixel,
         float freq, const std::string& name)
    : _pio(pio), _pin(pin), _num_leds(num_leds),
      _bits_per_pixel(bits_per_pixel), _name(name) {

  // Load the PIO program into the PIO memory
  int offset = _program_offset_pio[_pio->getPIONumber()];

  if (offset < 0) {
    if(_pio->addProgram(&led_program)){
      offset = _pio->getProgramOffset();
    }
  }else{
    _pio->setProgramOffset(offset);
  }
  if(offset < 0) {
    _status = DeviceStatus::Error;
    return;
  }
  _program_offset_pio[_pio->getPIONumber()] = offset;
  if (_num_leds > DMA_THRESHOLD) {
    if(!_pio->useDMA32(_num_leds)) {
      _status = DeviceStatus::Error;
      return;
    }
  }

  led_program_init(_pio->getPIO(), _pio->getSM(), offset, _pin, freq, _bits_per_pixel);

  _status = DeviceStatus::Initialized;
}

const std::string WS2812::getDetails() const {
  return "WS2812 LED strip on pin " + std::to_string(_pin) + 
         " with " + std::to_string(_num_leds) + " LEDs (" + 
         std::to_string(_bits_per_pixel) + " bits/pixel)";
}

bool WS2812::setPattern(const uint32_t* data, size_t count) {
  if (count < _num_leds) {
    return false; 
  }
  return _pio->transfer(data, _num_leds);
}
