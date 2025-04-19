#pragma once

#include "IHLKPackage.h"
#include <cstdint>
#include <memory>
#include <vector>

class HLKPackageFinder {
public:
  // Constructor
  HLKPackageFinder() = default;

  // Destructor
  ~HLKPackageFinder() = default;

  // Method to find a package in the buffer
  std::shared_ptr<IHLKPackage> findPackage(const uint8_t *buffer,
                                           const int size);

private:
  enum class State {
    WaitingForStart,
    WaitingForMinimal,
    WaitingForStandart,
    WaitingForCommand,
  };

  // Constants
  static constexpr uint32_t StandartFrameHead = 0xF4F3F2F1;
  static constexpr uint32_t StandartFrameTail = 0xF8F7F6F5;
  static constexpr uint32_t CommandFrameHead = 0xFDFCFBFA;
  static constexpr uint32_t CommandFrameTail = 0x04030201;

  State _state = State::WaitingForStart;
  uint32_t _pattern = 0;
  std::vector<uint8_t> _buffer;
};
