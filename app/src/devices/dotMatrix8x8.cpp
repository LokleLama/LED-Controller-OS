#include "devices/dotMatrix8x8.h"
#include "devices/MatrixChar8x8.h"
#include <cstring>
#include <iostream>
#include <vector>

dotMatrix8x8::dotMatrix8x8(std::shared_ptr<WS2812> led, const std::string& name, const std::string& start, uint32_t color)
    : IDisplayDevice(color), _led(led), _name(name) {

  setValue(start);

  _currentFrame.resize(64, 0); // 8x8 matrix = 64 LEDs

  _scrollingTask = Mainloop::getInstance().registerTimedTask("dotMatrixTextScrolling", [this]() { return scrollText(); }, 100);

  _status = DeviceStatus::Initialized;
}

const std::string dotMatrix8x8::getDetails() const {
  return "dotMatrix8x8 device with 64 LEDs (8x8 matrix) using WS2812 device: " + _led->getName();
}

void dotMatrix8x8::setValue(const std::string& value){
  if (value.length() < 1) {
    std::cerr << "Invalid value length for dotMatrix8x8 display: " << value << std::endl;
    return;
  }

  _bit_vector_length = (value.length() + 1) * 9; // Each character is 8 columns wide + 1 spacer column
  _current_offset = 0; // Start at the beginning of the LED data

  _total_columns = ((_bit_vector_length + 26) / 27);

  _ledData.resize(_total_columns * 8, 0); // Initialize all LEDs to off
  memset(_ledData.data(), 0, _ledData.size() * sizeof(uint32_t)); // Clear the LED data

  const char* str = value.c_str();
  for (size_t i = 0; i < value.length(); i++) {
    int data_index = i / 3;
    int bit_offset = (i - (data_index * 3)) * 9;

    const uint8_t* charData = MatrixChar8x8::getChar(str[i]);
    for (int col = 0; col < 8; col++) {
      uint32_t columnData = charData[col];
      columnData = (columnData & 0xFF) << bit_offset; // Shift to correct position in the LED data

      _ledData[data_index + col * _total_columns] |= columnData; // Combine with existing data for this column
    }
  }
  {
    int data_index = value.length() / 3;
    int bit_offset = (value.length() - (data_index * 3)) * 9;

    const uint8_t* charData = MatrixChar8x8::getChar(str[0]);
    for (int col = 0; col < 8; col++) {
      uint32_t columnData = charData[col];
      columnData = (columnData & 0xFF) << bit_offset; // Shift to correct position in the LED data

      _ledData[data_index + col * _total_columns] |= columnData; // Combine with existing data for this column
    }
  }
}

void dotMatrix8x8::setScrollingSpeed(int speed) {
  Mainloop::getInstance().modifyTimedTaskInterval(_scrollingTask, speed);
}

bool dotMatrix8x8::scrollText() {
  int startIndex = _current_offset / 27;
  int endIndex = (_current_offset + 7) / 27;

  if(startIndex == endIndex) {
    int bit_offset = _current_offset - startIndex * 27;
    for (int col = 0; col < 8; col++) {
      uint32_t columnData = _ledData[startIndex + col * _total_columns] >> bit_offset;
      for (int row = 0; row < 8; row++) {
        _currentFrame[row + col * 8] = (columnData & 0x01) ? _color : 0x00000000;
        columnData >>= 1;
      }
    }
  }else{
    // Handle the case where the text spans across two columns
    int bit_offset_start = _current_offset - startIndex * 27;
    int bit_offset_end = 27 - bit_offset_start;

    for (int col = 0; col < 8; col++) {
      uint32_t columnDataStart = _ledData[startIndex + col * _total_columns] >> bit_offset_start;
      uint32_t columnDataEnd = _ledData[endIndex + col * _total_columns] << bit_offset_end;
      uint32_t columnData = columnDataStart | columnDataEnd;

      for (int row = 0; row < 8; row++) {
        _currentFrame[row + col * 8] = (columnData & 0x01) ? _color : 0x00000000;
        columnData >>= 1;
      }
    }
  }

  _led->setPattern(_currentFrame);
  _current_offset++;
  if ((_current_offset + 8) >= _bit_vector_length) {
    _current_offset = 0; // Loop back to the beginning
  }
  return true;
}
