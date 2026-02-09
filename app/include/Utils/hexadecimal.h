#pragma once

#include <cstdint>
#include <string>
#include <vector>

class Hexadecimal {
public:
  // Encodes a uint8_t array to a hexadecimal string
  static std::string encode(const std::vector<uint8_t> &data);

  // Decodes a hexadecimal string to a uint8_t array
  static std::vector<uint8_t> decode(const std::string &hexString);
};
