#pragma once

#include "IHLKPackage.h"
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

class HLKCommand : public IHLKPackage {
public:
  HLKCommand(const uint16_t command, const uint8_t *parameter, const int size);

  // Override getSize method
  const int getSize() const override;

  // Override serialize method
  const int serialize(uint8_t *buffer, int size) const override;

  const std::string toString() const override;

  // Deserialization method
  static std::shared_ptr<HLKCommand> deserialize(const uint8_t *buffer,
                                                 const int size);

private:
  uint16_t _command;
  std::vector<uint8_t> _parameter;
};