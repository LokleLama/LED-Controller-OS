#include "HLKStack/HLKStandart.h"
#include <iomanip>
#include <memory>
#include <sstream>
#include <vector>

HLKStandart::HLKStandart(const uint8_t type, const uint8_t *data,
                         const int size)
    : _type(type), _data(data, data + size) {}

// Override getSize method
const int HLKStandart::getSize() const { return _data.size(); }

// Override serialize method
const int HLKStandart::serialize(uint8_t *buffer, int size) const {
  if (size < getSize() + 1 + 2 + 4 + 4) {
    return -1; // Not enough space in the buffer
  }
  buffer[0] = static_cast<uint8_t>(Type::Standard); // Type
  buffer[1] = 0xF3;
  buffer[2] = 0xF2;
  buffer[3] = 0xF1;

  uint16_t length = getSize();
  buffer[4] = static_cast<uint8_t>(length & 0xFF); // Low byte of length
  buffer[5] = static_cast<uint8_t>(length >> 8);   // High byte of length
  buffer[6] = _type;

  for (int i = 0; i < length; ++i) {
    buffer[7 + i] = _data[i]; // Copy data
  }

  buffer[7 + length] = 0xF8; // End byte
  buffer[8 + length] = 0xF7;
  buffer[9 + length] = 0xF6;
  buffer[10 + length] = 0xF5;

  return getSize() + 11; // Return the total size of the serialized data
}

const std::string HLKStandart::toString() const {
  std::stringstream result; // Create a single stringstream
  result << "{type = " << std::hex << std::uppercase << std::setw(2)
         << std::setfill('0') << static_cast<int>(_type) << ", data = [";

  for (size_t i = 0; i < _data.size(); ++i) {
    result << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(_data[i]) << " ";
  }
  result << "]}";
  return result.str();
}

// Deserialization method
std::shared_ptr<HLKStandart> HLKStandart::deserialize(const uint8_t *buffer,
                                                      const int size) {
  if (size < 10) {
    return nullptr;
  }
  if (buffer[0] != static_cast<uint8_t>(Type::Standard)) {
    return nullptr;
  }
  if (buffer[1] != 0xF3 || buffer[2] != 0xF2 || buffer[3] != 0xF1) {
    return nullptr;
  }
  uint16_t length = static_cast<uint16_t>(buffer[4]) |
                    (static_cast<uint16_t>(buffer[5]) << 8);
  if (length + 10 > size) {
    return nullptr;
  }
  uint8_t type = buffer[6];
  std::vector<uint8_t> data(buffer + 7, buffer + 7 + length - 1);
  if (buffer[7 + length - 1] != 0xF8 || buffer[8 + length - 1] != 0xF7 ||
      buffer[9 + length - 1] != 0xF6 || buffer[10 + length - 1] != 0xF5) {
    return nullptr;
  }
  return std::make_shared<HLKStandart>(type, data.data(), length);
}
