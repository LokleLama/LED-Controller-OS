#pragma once

#include <map>
#include <string>
#include <vector>

class Flash {
public:
  static int read(std::vector<uint8_t> &buffer, int offset);
  static int write(const std::vector<uint8_t> &buffer, int offset);
  static int erase(int offset, int length);
};
