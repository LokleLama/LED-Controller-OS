#pragma once

#include "IHLKPackage.h"
#include <cstdint>
#include <memory>
#include <stdexcept>

class HLKDistance : public IHLKPackage {
public:
  HLKDistance(uint8_t targetState, uint16_t distanceInCm);

  const Type getType() const override { return IHLKPackage::Type::Minimal; }

  // Getter for distance in meters
  float getDistance() const;

  bool getObjectDetected() const;

  // Override getSize method
  const int getSize() const override;

  // Override serialize method
  const int serialize(uint8_t *buffer, int size) const override;

  const std::string toString() const override;

  // Deserialization method
  static std::shared_ptr<HLKDistance> deserialize(const uint8_t *buffer,
                                                  const int size);

private:
  uint8_t targetState;
  uint16_t distanceInCm;
};