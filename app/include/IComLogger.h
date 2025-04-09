#pragma once

#include <cstdint>

class IComLogger {
public:
  virtual ~IComLogger() = default;

  // Returns the name of the command
  virtual void Transmitting(const uint8_t *buffer, int count) = 0;
  virtual void Receiving(const uint8_t *buffer, int count) = 0;
};