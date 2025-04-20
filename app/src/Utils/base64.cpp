#include "Utils/base64.h"
#include <cstdint>

const std::string Base64::BASE64_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                         "abcdefghijklmnopqrstuvwxyz"
                                         "0123456789+/";

std::string Base64::encode(const std::vector<uint8_t> &data) {
  std::string encodedString;
  size_t n = 0;
  uint32_t block;

  for (; n < (data.size() - 2); n += 3) {
    for (size_t i = 0; i < 3; ++i) {
      block <<= 8;
      block |= data[n + i];
    }
    encodedString += BASE64_CHARS[(block >> 18) & 0x3F];
    encodedString += BASE64_CHARS[(block >> 12) & 0x3F];
    encodedString += BASE64_CHARS[(block >> 6) & 0x3F];
    encodedString += BASE64_CHARS[block & 0x3F];
  }

  if (n < data.size()) {
    size_t i = 0;
    for (; n < data.size(); n++, i++) {
      block <<= 8;
      block |= data[n];
    }
    for (size_t j = i; j < 3; j++) {
      block <<= 8;
    }
    encodedString += BASE64_CHARS[(block >> 18) & 0x3F];
    encodedString += BASE64_CHARS[(block >> 12) & 0x3F];
    encodedString += (i == 2) ? BASE64_CHARS[(block >> 6) & 0x3F] : '=';
    encodedString += '=';
  }

  return encodedString;
}

const std::vector<uint8_t> Base64::BASE64_REVERSE = []() {
  std::vector<uint8_t> reverse(128, 0xFF);
  for (size_t i = 0; i < BASE64_CHARS.size(); ++i) {
    reverse[static_cast<uint8_t>(BASE64_CHARS[i])] = static_cast<uint8_t>(i);
  }
  reverse[static_cast<uint8_t>('=')] = 0;
  return reverse;
}();

std::vector<uint8_t> Base64::decode(const std::string &base64String) {
  int length = base64String.size();
  if (length % 4 != 0) {
    return {}; // Invalid Base64 string
  }

  std::vector<uint8_t> decodedData;
  uint32_t block;

  while (length > 0 && base64String[length - 1] == '=') {
    length--;
  }

  for (int n = 0; n < length;) {
    for (int m = 0; m < 4; ++m, n++) {
      block <<= 6;
      block |= BASE64_REVERSE[static_cast<uint8_t>(base64String[n])];
    }
    int skip = n - length;
    if (skip <= 0) {
      decodedData.push_back(static_cast<uint8_t>((block >> 16) & 0xFF));
      decodedData.push_back(static_cast<uint8_t>((block >> 8) & 0xFF));
      decodedData.push_back(static_cast<uint8_t>(block & 0xFF));
    } else {
      decodedData.push_back(static_cast<uint8_t>((block >> 16) & 0xFF));
      if (skip >= 1) {
        decodedData.push_back(static_cast<uint8_t>((block >> 8) & 0xFF));
      }
    }
  }

  return decodedData;
}