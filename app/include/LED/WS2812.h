#pragma once

#include "hardware/clocks.h"
#include "hardware/pio.h"

#include "PIO/led.pio.h"
#include <cstdint>
#include <vector>

class WS2812 {
public:
  WS2812(PIO pio, uint pin, uint num_leds, uint bits_per_pixel = 24,
         float freq = 800000);

  // Set the pattern for the LEDs
  // The pattern is a vector of uint32_t, where each uint32_t represents
  /// returns true if the pattern was set successfully
  /// returns false if the pattern size was less than the number of LEDs
  /// returns false if the DMA is already busy
  bool setPattern(const std::vector<uint32_t> &pattern);

  // Add a pattern to the pattern memory
  // The pattern is a vector of uint32_t, where each uint32_t represents
  /// returns the pattern index
  /// returns -1 if the pattern size was less than the number of LEDs
  int addPattern(const std::vector<uint32_t> &pattern);
  // Set the pattern for the LEDs
  /// returns true if the pattern was set successfully
  bool setPattern(int pattern);
  // removed the pattern from the pattern memory
  /// returns true if the pattern was removed successfully
  /// returns false if the pattern was not found
  bool removePattern(int pattern);

private:
  PIO _pio;
  uint _pin;
  uint _num_leds;
  uint _bits_per_pixel;

  uint _sm;
  uint _dma_channel;

  std::vector<std::vector<uint32_t>> _patterns;

  static constexpr int DMA_THRESHOLD = 16;

  static int _program_offset_pio0;
  static int _program_offset_pio1;
};