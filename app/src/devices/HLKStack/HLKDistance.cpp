#include "HLKStack/HLKDistance.h"
#include <memory>

HLKDistance::HLKDistance(uint8_t targetState, uint16_t distanceInCm)
    : targetState(targetState), distanceInCm(distanceInCm) {}

// Getter for distance in meters
float HLKDistance::getDistance() const {
  return static_cast<float>(distanceInCm) / 100.0;
}

bool HLKDistance::getObjectDetected() const {
  return targetState & 0x02 != 0; // Assuming 0x01 indicates object detected
}

// Override getSize method
const int HLKDistance::getSize() const { return 5; }

// Override serialize method
const int HLKDistance::serialize(uint8_t *buffer, int size) const {
  if (size < 5) {
    return -1; // Not enough space in the buffer
  }

  buffer[0] = IHLKPackage::MinimalFrameHead;             // Type
  buffer[1] = targetState;                               // Target state
  buffer[2] = static_cast<uint8_t>(distanceInCm & 0xFF); // Low byte of distance
  buffer[3] = static_cast<uint8_t>(distanceInCm >> 8); // High byte of distance
  buffer[4] = IHLKPackage::MinimalFrameTail;           // End byte

  return 5;
}

const std::string HLKDistance::toString() const {
  return "{distance = " + std::to_string(getDistance()) +
         "m (detected=" + std::to_string(getObjectDetected()) + ")}";
}

// Deserialization method
std::shared_ptr<HLKDistance> HLKDistance::deserialize(const uint8_t *buffer,
                                                      const int size) {
  if (size < 5) {
    return nullptr;
  }

  if (buffer[0] != IHLKPackage::MinimalFrameHead) {
    return nullptr;
  }

  if (buffer[4] != IHLKPackage::MinimalFrameTail) {
    return nullptr;
  }

  uint8_t targetState = buffer[1];
  uint16_t distanceInCm = static_cast<uint16_t>(buffer[2]) |
                          (static_cast<uint16_t>(buffer[3]) << 8);

  return std::make_shared<HLKDistance>(targetState, distanceInCm);
}