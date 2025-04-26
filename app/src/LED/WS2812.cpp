#include "LED/WS2812.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include <cstring>
#include <vector>

int WS2812::_program_offset_pio0 = -1;
int WS2812::_program_offset_pio1 = -1;

WS2812::WS2812(PIO pio, uint pin, uint num_leds, uint bits_per_pixel,
               float freq)
    : _pio(pio), _pin(pin), _num_leds(num_leds),
      _bits_per_pixel(bits_per_pixel) {
  // Claim a state machine
  _sm = pio_claim_unused_sm(_pio, true);

  // Load the PIO program into the PIO memory
  uint offset = 0;
  if (_pio == pio0) {
    if (_program_offset_pio0 == -1) {
      _program_offset_pio0 = pio_add_program(_pio, &led_program);
    }
    offset = _program_offset_pio0;
    if (_num_leds > DMA_THRESHOLD) {
      _dma_channel = dma_claim_unused_channel(false);
    }
  } else if (_pio == pio1) {
    if (_program_offset_pio1 == -1) {
      _program_offset_pio1 = pio_add_program(_pio, &led_program);
    }
    offset = _program_offset_pio1;
    if (_num_leds > DMA_THRESHOLD) {
      _dma_channel = dma_claim_unused_channel(false);
    }
  }

  led_program_init(_pio, _sm, offset, _pin, freq, _bits_per_pixel);

  if (_dma_channel >= 0) {
    dma_channel_config dc = dma_channel_get_default_config(_dma_channel);
    channel_config_set_transfer_data_size(&dc, DMA_SIZE_32);
    channel_config_set_read_increment(&dc, true);
    channel_config_set_write_increment(&dc, false);
    channel_config_set_dreq(&dc, pio_get_dreq(_pio, _sm, true));
    dma_channel_configure(_dma_channel, &dc, &pio->txf[_sm], nullptr, _num_leds,
                          false);
  }
}

bool WS2812::setPattern(const std::vector<uint32_t> &pattern) {
  if (pattern.size() < _num_leds) {
    return false; // Pattern size exceeds the number of LEDs
  }

  if (_dma_channel < 0) {
    // If DMA is not used, we can directly set the pattern
    for (size_t i = 0; i < _num_leds; ++i) {
      pio_sm_put_blocking(_pio, _sm, pattern[i]);
    }
  } else {
    if (dma_channel_is_busy(_dma_channel)) {
      return false; // DMA is busy
    }

    // Start the DMA transfer
    dma_channel_transfer_from_buffer_now(_dma_channel, pattern.data(),
                                         _num_leds);
  }
  return true;
}

int WS2812::addPattern(const std::vector<uint32_t> &pattern) { return -1; }

bool WS2812::setPattern(int pattern) {
  if (pattern < 0 || pattern >= _patterns.size()) {
    return false; // Invalid pattern index
  }

  return setPattern(_patterns[pattern]);
}

bool WS2812::removePattern(int pattern) { return false; }
