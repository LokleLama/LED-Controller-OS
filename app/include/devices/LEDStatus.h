#pragma once

#include "devices/IDevice.h"
#include "devices/WS2812.h"

#include <cstdint>
#include <string>
#include <map>
#include <memory>
#include <vector>

class LEDStatus : public ICreateSharedFromThis<LEDStatus>, public IDevice {
public:
  LEDStatus(std::shared_ptr<WS2812> led, const std::string& name = "LEDStatus", const std::string& initial_status = "Idle");

  const std::string getName() const override { return _name; }
  const std::string getType() const override { return "LEDStatus"; }
  const std::string getDetails() const override;

  bool setStatus(const std::string& value);

  void ClearMap() {
    _status_colors.clear();
  }

  void AddStatusColor(const std::string& status, uint32_t color) {
    _status_colors[status] = color;
  }
private:
    std::shared_ptr<WS2812> _led;
    std::string _name;

    std::map<std::string, uint32_t> _status_colors = {
        {"OK", 0x00030000},       // Green
        {"Warning", 0x03030000},  // Yellow
        {"Error", 0x03000000},    // Red
        {"Idle", 0x00000300},     // Blue
    };

    std::vector<uint32_t> _led_colors;
};