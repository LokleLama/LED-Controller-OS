#include "HLKStack/HLKFrame.h"
#include <iomanip>
#include <memory>
#include <sstream>
#include <vector>

HLKFrame::HLKFrame(const uint16_t command, const uint8_t *parameter,
                   const int size)
    : _command(command), _parameter(parameter, parameter + size) {}

// Override getSize method
const int HLKFrame::getSize() const { return _parameter.size(); }

// Override serialize method
const int HLKFrame::serialize(uint8_t *buffer, int size) const {
  if (size < getSize() + 2 + 2 + 4 + 4) {
    return -1; // Not enough space in the buffer
  }
  buffer[0] = static_cast<uint8_t>(Type::Frame); // Type
  buffer[1] = 0xFC;
  buffer[2] = 0xFB;
  buffer[3] = 0xFA;

  uint16_t length = getSize();
  buffer[4] = static_cast<uint8_t>(length & 0xFF); // Low byte of length
  buffer[5] = static_cast<uint8_t>(length >> 8);   // High byte of length

  buffer[6] = static_cast<uint8_t>(_command & 0xFF); // Low byte of command
  buffer[7] = static_cast<uint8_t>(_command >> 8);   // High byte of command

  for (int i = 0; i < length; ++i) {
    buffer[8 + i] = _parameter[i]; // Copy parameter data
  }

  buffer[8 + length] = 0x04; // End byte
  buffer[9 + length] = 0x03;
  buffer[10 + length] = 0x02;
  buffer[11 + length] = 0x01;

  return getSize() + 12; // Return the total size of the serialized data
}

const std::string HLKFrame::toString() const {
  std::stringstream result; // Create a single stringstream
  result << "{command = " << std::hex << std::uppercase << std::setw(4)
         << std::setfill('0') << static_cast<int>(_command)
         << ", parameter = [";

  for (size_t i = 0; i < _parameter.size(); ++i) {
    result << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(_parameter[i]) << " ";
  }
  result << "]}";
  return result.str();
}

// Deserialization method
std::shared_ptr<HLKFrame> HLKFrame::deserialize(const uint8_t *buffer,
                                                const int size) {
  if (size < 12) {
    return nullptr;
  }
  if (buffer[0] != static_cast<uint8_t>(Type::Frame)) {
    return nullptr;
  }
  if (buffer[1] != 0xFC || buffer[2] != 0xFB || buffer[3] != 0xFA) {
    return nullptr;
  }
  uint16_t length = static_cast<uint16_t>(buffer[4]) |
                    (static_cast<uint16_t>(buffer[5]) << 8);
  if (length + 12 > size) {
    return nullptr;
  }
  uint16_t command = static_cast<uint16_t>(buffer[6]) |
                     (static_cast<uint16_t>(buffer[7]) << 8);
  std::vector<uint8_t> parameter(buffer + 8, buffer + 8 + length);
  if (buffer[8 + length] != 0x04 || buffer[9 + length] != 0x03 ||
      buffer[10 + length] != 0x02 || buffer[11 + length] != 0x01) {
    return nullptr;
  }
  return std::make_shared<HLKFrame>(command, parameter.data(), length);
}