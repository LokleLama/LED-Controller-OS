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

class dotMatrix8xN : public IDisplayDevice, public IDisplayScrolling, public std::enable_shared_from_this<dotMatrix8xN> {
public:
  dotMatrix8xN(std::shared_ptr<WS2812> led, const std::string& name = "dotMatrix8xN", const std::string& start = " ", uint32_t color = 0x03030303);

  const std::string getName() const override { return _name; }
  const std::string getType() const override { return "dotMatrix8xN"; }
  const std::string getDetails() const override;

  void setValue(const std::string& value) override;
  void setScrollingSpeed(int speed) override;
  void setScrollingDirection(ScrollingDirection direction) override { _scrollingDirection = direction; _current_offset = 0; }

private:
  std::shared_ptr<WS2812> _led;
  std::string _name;
  TaskPID _scrollingTask;
  ScrollingDirection _scrollingDirection = ScrollingDirection::LEFT;

  std::vector<uint8_t> _ledData;
  std::vector<uint32_t> _currentFrame;
  
  int _current_offset;

  bool scrollText();
  bool staticText();
};