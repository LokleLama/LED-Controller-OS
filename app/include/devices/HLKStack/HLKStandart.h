#pragma once

#include "IHLKPackage.h"
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

class HLKStandart : public IHLKPackage {
public:
  HLKStandart(const uint8_t type, const uint8_t *data, const int size);

  const Type getType() const override { return IHLKPackage::Type::Standart; }

  // Override getSize method
  const int getSize() const override;

  // Override serialize method
  const int serialize(uint8_t *buffer, int size) const override;

  const std::string toString() const override;

  // Deserialization method
  static std::shared_ptr<HLKStandart> deserialize(const uint8_t *buffer,
                                                  const int size);

private:
  uint8_t _type;
  std::vector<uint8_t> _data;
};