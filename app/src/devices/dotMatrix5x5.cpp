#include "devices/dotMatrix5x5.h"
#include "devices/MatrixChar5x5.h"
#include <cstring>
#include <iostream>
#include <vector>

dotMatrix5x5::dotMatrix5x5(std::shared_ptr<WS2812> led, const std::string& name, const std::string& start)
    : _led(led), _name(name) {

  _color = 0x03030303;
  setValue("Hello :)");

  _currentFrame.resize(25, 0); // 5x5 matrix = 25 LEDs

  _scrollingTask = Mainloop::getInstance().registerTimedTask("dotMatrixTextScrolling", [this]() { return scrollText(); }, 100);

  _status = DeviceStatus::Initialized;
}

const std::string dotMatrix5x5::getDetails() const {
  return "dotMatrix5x5 device with 25 LEDs (5x5 matrix) using WS2812 device: " + _led->getName();
}

void dotMatrix5x5::setValue(const std::string& value){
  if (value.length() < 1) {
    std::cerr << "Invalid value length for dotMatrix5x5 display: " << value << std::endl;
    return;
  }

  _bit_vector_length = (value.length() + 2) * 6; // Each character is 5 columns wide + 1 spacer column
  _current_offset = 0; // Start at the beginning of the LED data

  _total_columns = ((_bit_vector_length + 29) / 30);

  _ledData.resize(_total_columns * 5, 0); // Initialize all LEDs to off

  const char* str = value.c_str();
  for (size_t i = 0; i < value.length(); i++) {
    int data_index = (i + 1) / 5;
    int bit_offset = ((i + 1) - (data_index * 5)) * 6;

    const uint8_t* charData = MatricChar5x5::getChar(str[i]);

    for (int col = 0; col < 5; col++) {
      uint32_t columnData = charData[col];
      columnData = (columnData & 0x1F) << bit_offset; // Shift to correct position in the LED data

      _ledData[data_index + col * _total_columns] |= columnData; // Combine with existing data for this column
    }
  }
}

void dotMatrix5x5::setScrollingSpeed(int speed) {
  Mainloop::getInstance().modifyTimedTaskInterval(_scrollingTask, speed);
}

bool dotMatrix5x5::scrollText() {
  int startIndex = _current_offset / 30;
  int endIndex = (_current_offset + 5) / 30;

  if(startIndex == endIndex) {
    int bit_offset = startIndex * 30 - _current_offset;
    for (int col = 0; col < 5; col++) {
      uint32_t columnData = _ledData[startIndex + col * _total_columns] >> bit_offset;
      for (int row = 0; row < 5; row++) {
        _currentFrame[row * 5 + col] = (columnData & 0x01) ? _color : 0x00000000;
        columnData >>= 1;
      }
    }
  }else{
    
  }

  _led->setPattern(_currentFrame);
  _current_offset++;
  if (_current_offset >= _bit_vector_length) {
    _current_offset = 0; // Loop back to the beginning
  }
  return true;
}
