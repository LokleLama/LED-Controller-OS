#pragma once

#include "HLKStack/HLKPackageFinder.h"
#include "IComLogger.h"

#include <iomanip>
#include <iostream>
#include <string>

class HLKLogger : public IComLogger {
public:
  HLKLogger() = default;

  void Transmitting(const uint8_t *buffer, int count) override {
    auto pack = _trasnmitterFinder.findPackage(buffer, count);
    if (pack) {
      std::cout << "Transmitting: " << pack->toString() << std::endl;
    }
  }
  void Receiving(const uint8_t *buffer, int count) override {
    auto pack = _receiverFinder.findPackage(buffer, count);
    if (pack) {
      std::cout << "Receiving: " << pack->toString() << std::endl;
    }
  }

private:
  HLKPackageFinder _trasnmitterFinder;
  HLKPackageFinder _receiverFinder;
};