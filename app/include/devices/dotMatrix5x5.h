#pragma once

#include "devices/IDisplayDevice.h"
#include "devices/WS2812.h"

#include "devices/MatrixChar5x5.h"

#include "Mainloop.h"

#include <cstdint>
#include <vector>
#include <memory>
#include <string>

class dotMatrix5x5 : public IDisplayDevice, public std::enable_shared_from_this<dotMatrix5x5> {
public:
  dotMatrix5x5(std::shared_ptr<WS2812> led, const std::string& name = "dotMatrix5x5", const std::string& start = " ");

  const std::string getName() const override { return _name; }
  const std::string getType() const override { return "dotMatrix5x5"; }
  const std::string getDetails() const override;

  void setValue(const std::string& value) override;
  void setScrollingSpeed(int speed);

  std::shared_ptr<dotMatrix5x5> getShared() {
      return shared_from_this();
  }

private:
  std::shared_ptr<WS2812> _led;
  std::string _name;
  Mainloop::TaskHandle _scrollingTask;

  std::vector<uint32_t> _ledData;
  std::vector<uint32_t> _currentFrame;
  int _total_columns;
  int _current_offset;
  int _bit_vector_length;

  bool scrollText();
};