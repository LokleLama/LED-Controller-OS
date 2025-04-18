#pragma once

#include <cstdint>
#include <string>

class IHLKPackage {
public:
  enum class Type { Minimal = 0x6E, Standard = 0xF4, Frame = 0xFD };

  virtual ~IHLKPackage() = default;

  virtual const int getSize() const = 0;

  // Serializes the object into a byte array
  virtual const int serialize(uint8_t *buffer, int size) const = 0;

  virtual const std::string toString() const = 0;
};
