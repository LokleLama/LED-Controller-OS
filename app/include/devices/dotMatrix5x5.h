#pragma once

#include "devices/IDisplayDevice.h"
#include "devices/IDisplayScrolling.h"
#include "devices/WS2812.h"

#include "devices/MatrixChar5x5.h"

#include "Mainloop.h"

#include <cstdint>
#include <vector>
#include <memory>
#include <string>

class dotMatrix5x5 : public ICreateSharedFromThis<dotMatrix5x5>, public IDisplayDevice, public IDisplayScrolling {
public:
  dotMatrix5x5(std::shared_ptr<WS2812> led, const std::string& name = "dotMatrix5x5", const std::string& start = " ", uint32_t color = 0x03030303);

  const std::string getName() const override { return _name; }
  const std::string getType() const override { return "dotMatrix5x5"; }
  const std::string getDetails() const override;

  void setValue(const std::string& value) override;
  void setScrollingSpeed(int speed) override;
  void setScrollingDirection(ScrollingDirection direction) override { _scrollingDirection = direction; _current_offset = 0; }

private:
  std::shared_ptr<WS2812> _led;
  std::string _name;
  Mainloop::TaskHandle _scrollingTask;

  std::vector<uint32_t> _ledData;
  std::vector<uint32_t> _currentFrame;
  int _total_columns;
  int _current_offset;
  int _bit_vector_length;
  ScrollingDirection _scrollingDirection = ScrollingDirection::LEFT;

  bool scrollText();
};