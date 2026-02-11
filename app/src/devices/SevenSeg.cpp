#include "devices/SevenSeg.h"
#include "devices/WS2812.h"
#include <cstring>
#include <iostream>
#include <vector>

SevenSeg::SevenSeg(std::shared_ptr<WS2812> led, const std::string& name, const std::string& start)
    : _led(led), _name(name) {

  _color = 0x03030303;
  setValue(start);

  _status = DeviceStatus::Initialized;
}

const std::string SevenSeg::getDetails() const {
  return "7Seg device with 142 LEDs (4 digits * 7 segments * 5 LEDs per segment + 2 dot LEDs) using WS2812 device: " + _led->getName();
}

void SevenSeg::setValue(const std::string& value){
  if (value.length() < 5) {
    std::cerr << "Invalid value length for 7-segment display: " << value << std::endl;
    return;
  }

  auto ledPtr = setCharacter(_ledString, value[0]);
  ledPtr = setCharacter(ledPtr, value[1]);

    // Handle the dot/double dot/blank in the middle
  if (value[2] == '.') {
    *ledPtr = _color;
    ledPtr++;
    *ledPtr = 0x00000000;
    ledPtr++;
  } else if (value[2] == ':') {
    *ledPtr = _color;
    ledPtr++;
    *ledPtr = _color;
    ledPtr++;
  } else {
    *ledPtr = 0x00000000;
    ledPtr++;
    *ledPtr = 0x00000000;
    ledPtr++;
  }

  ledPtr = setCharacter(ledPtr, value[3]);
  setCharacter(ledPtr, value[4]);

  _led->setPattern(_ledString, 142);
}
  
uint32_t* SevenSeg::setCharacter(uint32_t* ledData, char c) {
  if(c < 0 || c >= 128) {
    c = ' '; // Treat out-of-range characters as space
  }
  uint8_t segments = ASCII_TO_7SEG[static_cast<uint8_t>(c)];
  for (int i = 0; i < 7; i++) {
    uint32_t color = (segments & 0x40) ? _color : 0x00000000;
    for (int j = 0; j < 5; j++) {
      *ledData = color;
      ledData++;
    }
    segments <<= 1;
  }
  return ledData;
}
