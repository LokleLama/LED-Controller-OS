#pragma once

#include <cstdint>

class IDataStream {
public:
  virtual ~IDataStream() = default;

  /// buffer is the pointer to the byte array where the data will be read into
  /// size is the size of the buffer
  /// Returns the number of bytes read
  /// Returns -1 on error
  virtual int readAvailable(uint8_t *buffer, uint32_t size) = 0;

  /// buffer is the pointer to the byte array containing the data to be written
  /// size is the size of the buffer
  /// Returns the number of bytes written
  /// Returns -1 on error
  virtual int writeAvailable(const uint8_t *buffer, uint32_t size) = 0;
};