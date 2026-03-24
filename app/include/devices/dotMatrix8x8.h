#pragma once

#include "devices/IDisplayDevice.h"
#include "devices/IDisplayScrolling.h"
#include "devices/WS2812.h"

#include "devices/MatrixChar8x8.h"

#include "Mainloop.h"

#include <cstdint>
#include <vector>
#include <memory>
#include <string>

class dotMatrix8x8 : public IDisplayDevice, public IDisplayScrolling, public std::enable_shared_from_this<dotMatrix8x8> {
public:
  dotMatrix8x8(std::shared_ptr<WS2812> led, const std::string& name = "dotMatrix8x8", const std::string& start = " ", uint32_t color = 0x03030303);

  const std::string getName() const override { return _name; }
  const std::string getType() const override { return "dotMatrix8x8"; }
  const std::string getDetails() const override;

  void setValue(const std::string& value) override;
  void setScrollingSpeed(int speed) override;

  int getDisplayColumns() const { return _num_columns; }

  std::shared_ptr<dotMatrix8x8> getShared() {
      return shared_from_this();
  }

private:
  std::shared_ptr<WS2812> _led;
  std::string _name;
  Mainloop::TaskHandle _scrollingTask;
  int _num_columns;  // display width in columns (8 per module)

  std::vector<uint32_t> _ledData;
  std::vector<uint32_t> _currentFrame;
  int _total_columns;
  int _current_offset;
  int _bit_vector_length;

  bool scrollText();
};