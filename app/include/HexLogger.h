#pragma once

#include "IComLogger.h"

#include <iomanip>
#include <iostream>
#include <string>

class HexLogger : public IComLogger {
public:
  HexLogger() = default;

  void Transmitting(const uint8_t *buffer, int count) override {
    if (count <= 0)
      return;
    std::cout << "send(";
    for (int i = 0; i < count; ++i) {
      std::cout << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(buffer[i]) << " ";
    }
    std::cout << ")" << std::dec << std::endl;
  }
  void Receiving(const uint8_t *buffer, int count) override {
    if (count <= 0)
      return;
    std::cout << "recv(";
    for (int i = 0; i < count; ++i) {
      std::cout << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(buffer[i]) << " ";
    }
    std::cout << ")" << std::dec << std::endl;
  }
};