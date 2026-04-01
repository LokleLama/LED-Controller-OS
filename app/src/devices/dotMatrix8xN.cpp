#include "devices/dotMatrix8xN.h"
#include "devices/MatrixChar8x8.h"
#include <cstring>
#include <iostream>
#include <vector>

dotMatrix8xN::dotMatrix8xN(std::shared_ptr<WS2812> led, const std::string& name, const std::string& start, uint32_t color)
    : IDisplayDevice(color), _led(led), _name(name) {

  setValue(start);

  _currentFrame.resize(led->getLEDCount() & ~0x07, 0);

  _scrollingTask = Mainloop::getInstance().registerTimedTask(name + ".TextScrolling", [this]() { return scrollText(); }, 100);

  _status = DeviceStatus::Initialized;
}

const std::string dotMatrix8xN::getDetails() const {
  return "dotMatrix8xN device with " + std::to_string(_currentFrame.size()) + " LEDs using WS2812 device: " + _led->getName();
}

void dotMatrix8xN::setValue(const std::string& value){
  if (value.length() < 1) {
    std::cerr << "Invalid value length for dotMatrix8xN display: " << value << std::endl;
    return;
  }
  _current_offset = 0;

  _ledData.clear();
  _ledData.reserve(value.length() * 8);

  for (size_t i = 0; i < value.length(); i++) {
    const uint8_t* charData = MatricChar8x8::getChar(value[i]);
    _ledData.insert(_ledData.end(), charData, charData + 8);
  }
}

void dotMatrix8xN::setScrollingSpeed(int speed) {
  Mainloop::getInstance().modifyTimedTaskInterval(_scrollingTask, speed);
}

bool dotMatrix8xN::scrollText() {
  if (_ledData.empty()) {
    return true;
  }

  if (_ledData.size() < _currentFrame.size() / 8) {
    return staticText();
  }
  
  int position = _current_offset;
  int LEDDirection = 0; // 0 = UpDown, 1 = DownUp

  for (size_t i = 0; i < _currentFrame.size(); position++) {
    if(position >= _ledData.size()) {
      position = 0;
    }

    uint8_t columnData = _ledData[position];
    if(LEDDirection == 0) {
      for(int bit = 0; bit < 8; bit++, i++) {
        _currentFrame[i] = (columnData & (1 << bit)) ? _color : 0x00000000;
      }
      LEDDirection = 1;
    }else{
      for(int bit = 7; bit >= 0; bit--, i++) {
        _currentFrame[i] = (columnData & (1 << bit)) ? _color : 0x00000000;
      }
      LEDDirection = 0;
    }
  }

  _led->setPattern(_currentFrame);

  _current_offset++;
  if (_current_offset >= _ledData.size()) {
    _current_offset = 0;
  }
  return true;
}

bool dotMatrix8xN::staticText() {
  int LEDDirection = 0; // 0 = UpDown, 1 = DownUp

  for (size_t i = 0, position = 0; position < _ledData.size(); position++) {
    uint8_t columnData = _ledData[position];
    if(LEDDirection == 0) {
      for(int bit = 0; bit < 8; bit++, i++) {
        _currentFrame[i] = (columnData & (1 << bit)) ? _color : 0x00000000;
      }
      LEDDirection = 1;
    }else{
      for(int bit = 7; bit >= 0; bit--, i++) {
        _currentFrame[i] = (columnData & (1 << bit)) ? _color : 0x00000000;
      }
      LEDDirection = 0;
    }
  }

  _led->setPattern(_currentFrame);
  return true;
}