#include "devices/dotMatrix8x8.h"
#include "devices/MatrixChar8x8.h"
#include <cstring>
#include <iostream>
#include <vector>

dotMatrix8x8::dotMatrix8x8(std::shared_ptr<WS2812> led, const std::string& name, const std::string& start, uint32_t color)
    : IDisplayDevice(color), _led(led), _name(name) {

  // Derive display width from the WS2812 LED count (8 rows per module)
  _num_columns = _led->getLEDCount() / 8;
  if (_num_columns < 8) _num_columns = 8;

  setValue(start);

  _currentFrame.resize(_num_columns * 8, 0);

  _scrollingTask = Mainloop::getInstance().registerTimedTask("dotMatrixTextScrolling", [this]() { return scrollText(); }, 100);

  _status = DeviceStatus::Initialized;
}

const std::string dotMatrix8x8::getDetails() const {
  return "dotMatrix8x8 device with " + std::to_string(_num_columns * 8) + " LEDs (8x" + std::to_string(_num_columns) + " matrix) using WS2812 device: " + _led->getName();
}

void dotMatrix8x8::setValue(const std::string& value){
  if (value.length() < 1) {
    std::cerr << "Invalid value length for dotMatrix8x8 display: " << value << std::endl;
    return;
  }

  _bit_vector_length = (value.length() + 1) * 9; // Each character is 8 columns wide + 1 spacer column
  _current_offset = 0; // Start at the beginning of the LED data

  _total_columns = ((_bit_vector_length + 26) / 27);

  _ledData.resize(_total_columns * 8, 0); // 8 row strips × _total_columns uint32_t each
  memset(_ledData.data(), 0, _ledData.size() * sizeof(uint32_t));

  const char* str = value.c_str();
  auto packChar = [&](size_t charIndex, const uint8_t* charData) {
    int data_index = charIndex / 3;
    int bit_offset = (charIndex - (data_index * 3)) * 9;

    // Pack each row's column bits directly into its row strip (font is row-major)
    for (int row = 0; row < 8; row++) {
      _ledData[data_index + row * _total_columns] |= ((uint32_t)charData[row] << bit_offset);
    }
  };

  for (size_t i = 0; i < value.length(); i++) {
    packChar(i, MatrixChar8x8::getChar(str[i]));
  }
  // Copy first character at the end for seamless wrapping
  packChar(value.length(), MatrixChar8x8::getChar(str[0]));
}

void dotMatrix8x8::setScrollingSpeed(int speed) {
  Mainloop::getInstance().modifyTimedTaskInterval(_scrollingTask, speed);
}

bool dotMatrix8x8::scrollText() {
  for (int display_col = 0; display_col < _num_columns; display_col++) {
    int col_offset = _current_offset + display_col;
    if (col_offset >= _bit_vector_length) {
      col_offset -= _bit_vector_length;
    }

    int dataIndex = col_offset / 27;
    int bitOffset = col_offset - dataIndex * 27;

    for (int row = 0; row < 8; row++) {
      uint32_t bit = (_ledData[dataIndex + row * _total_columns] >> bitOffset) & 0x01;
      // Zigzag correction: odd columns are wired bottom-to-top on WS2812 8x8 modules
      int led_row = (display_col & 1) ? (7 - row) : row;
      _currentFrame[led_row + display_col * 8] = bit ? _color : 0x00000000;
    }
  }


  _led->setPattern(_currentFrame);
  _current_offset++;
  if ((_current_offset + _num_columns) >= _bit_vector_length) {
    _current_offset = 0;
  }
  return true;
}
