#pragma once

#include <cstdint>
#include <string>

class IHLKPackage {
public:
  // Constants
  static constexpr uint8_t MinimalFrameHead = 0x6E;
  static constexpr uint8_t MinimalFrameTail = 0x62;
  static constexpr uint32_t StandartFrameHead = 0xF4F3F2F1;
  static constexpr uint32_t StandartFrameTail = 0xF8F7F6F5;
  static constexpr uint32_t CommandFrameHead = 0xFDFCFBFA;
  static constexpr uint32_t CommandFrameTail = 0x04030201;

  virtual ~IHLKPackage() = default;

  virtual const int getSize() const = 0;

  // Serializes the object into a byte array
  virtual const int serialize(uint8_t *buffer, int size) const = 0;

  virtual const std::string toString() const = 0;
};
