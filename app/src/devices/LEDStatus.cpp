#include "devices/LEDStatus.h"
#include <iostream>

LEDStatus::LEDStatus(std::shared_ptr<WS2812> led, const std::string& name, const std::string& initial_status)
    : _led(led), _name(name) {
  _led_colors.resize(_led->getLEDCount(), 0);
  setStatus(initial_status);
    
  _status = DeviceStatus::Initialized;
}

const std::string LEDStatus::getDetails() const{
  std::string details = "LEDStatus Device\n";
  details += "Status Colors:\n";
  for (const auto& pair : _status_colors) {
      details += "  " + pair.first + ": 0x" + std::to_string(pair.second) + "\n";
  }
  return details;
}

bool LEDStatus::setStatus(const std::string& value){
  auto it = _status_colors.find(value);
  if (it != _status_colors.end()) {
    _led_colors.assign(_led_colors.size(), it->second);
    _led->setPattern(_led_colors);
    return true;
  }

  std::cout << "Unknown status: " << value << ". Available statuses are: ";
  for (const auto& pair : _status_colors) {
      std::cout << pair.first << " ";
  }
  std::cout << std::endl;
  return false;  
}