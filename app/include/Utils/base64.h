#pragma once

#include <cstdint>
#include <string>
#include <vector>

class Base64 {
public:
  // Encodes a uint8_t array to a Base64 string
  static std::string encode(const std::vector<uint8_t> &data);

  // Decodes a Base64 string to a uint8_t array
  static std::vector<uint8_t> decode(const std::string &base64String);

private:
  static const std::string BASE64_CHARS;
  static const std::vector<uint8_t> BASE64_REVERSE;
};
