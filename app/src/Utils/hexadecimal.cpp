#include "Utils/hexadecimal.h"
#include <cstdint>
#include <sstream>
#include <iomanip>

std::string Hexadecimal::encode(const std::vector<uint8_t> &data) {
  std::stringstream ss;
  for (uint8_t byte : data) {
    ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte) << " ";
  }
  return ss.str();
}

std::vector<uint8_t> Hexadecimal::decode(const std::string &hexString) {
  std::vector<uint8_t> data;
  for (size_t i = 0; i < hexString.length(); i++) {
    if (hexString[i] == ' ') {
      continue; // Skip spaces
    }
    std::string byteString(1, hexString[i]);
    if (i + 1 < hexString.length() && hexString[i + 1] != ' ') {
      byteString += hexString[i + 1];
    } else {
      byteString = "0" + byteString;
    }

    uint8_t byte = static_cast<uint8_t>(std::stoi(byteString, nullptr, 16));
    data.push_back(byte);
    i += 1; // Move to the next byte (2 characters)
  }
  return data;
}
