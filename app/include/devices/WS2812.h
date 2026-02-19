#pragma once

#include "devices/IDevice.h"
#include "devices/PIODevice.h"

#include <cstdint>
#include <vector>
#include <memory>
#include <string>

class WS2812 : public IDevice, public std::enable_shared_from_this<WS2812> {
public:
  WS2812(std::shared_ptr<PIODevice> pio, uint pin, uint num_leds, uint bits_per_pixel = 24,
         float freq = 800000, const std::string& name = "WS2812");

  const std::string getName() const override { return _name; }
  const std::string getType() const override { return "WS2812"; }
  const std::string getDetails() const override;

  // Set the pattern for the LEDs
  // The pattern is a vector of uint32_t, where each uint32_t represents
  /// returns true if the pattern was set successfully
  /// returns false if the pattern size was less than the number of LEDs
  /// returns false if the DMA is already busy
  bool setPattern(const std::vector<uint32_t> &pattern){
    return setPattern(pattern.data(), pattern.size());
  }
  bool setPattern(const uint32_t* data, size_t count);

  std::shared_ptr<WS2812> getShared() {
      return shared_from_this();
  }

private:
  std::shared_ptr<PIODevice> _pio;
  uint8_t _pin;
  uint8_t _bits_per_pixel;
  uint _num_leds;
  std::string _name;

  static constexpr int DMA_THRESHOLD = 16;

  static int _program_offset_pio[2];
};