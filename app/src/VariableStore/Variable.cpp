#include "VariableStore/Variable.h"
#include <variant>

// Convert to string
std::string Variable::toString() const {
  return std::visit(
      [](const auto &val) -> std::string {
        if constexpr (std::is_same_v<decltype(val), std::string>) {
          return val;
        } else if constexpr (std::is_same_v<decltype(val), bool>) {
          return val ? "true" : "false";
        } else {
          return ""; // std::to_string(val);
        }
      },
      value_);
}

// Convert to float
float Variable::toFloat() const {
  return std::visit(
      [](const auto &val) -> float {
        if constexpr (std::is_same_v<decltype(val), float>) {
          return val;
        } else if constexpr (std::is_same_v<decltype(val), int>) {
          return static_cast<float>(val);
        } else if constexpr (std::is_same_v<decltype(val), bool>) {
          return val ? 1.0f : 0.0f;
        } else if constexpr (std::is_same_v<decltype(val), std::string>) {
          return std::stof(val);
        } else {
          return 0.0f;
        }
      },
      value_);
}

// Convert to int
int Variable::toInt() const {
  return std::visit(
      [](const auto &val) -> int {
        if constexpr (std::is_same_v<decltype(val), int>) {
          return val;
        } else if constexpr (std::is_same_v<decltype(val), float>) {
          return static_cast<int>(val);
        } else if constexpr (std::is_same_v<decltype(val), bool>) {
          return val ? 1 : 0;
        } else if constexpr (std::is_same_v<decltype(val), std::string>) {
          return std::stoi(val);
        } else {
          return 0;
        }
      },
      value_);
}

// Convert to bool
bool Variable::toBool() const {
  return std::visit(
      [](const auto &val) -> bool {
        if constexpr (std::is_same_v<decltype(val), bool>) {
          return val;
        } else if constexpr (std::is_same_v<decltype(val), int>) {
          return val != 0;
        } else if constexpr (std::is_same_v<decltype(val), float>) {
          return val != 0.0f;
        } else if constexpr (std::is_same_v<decltype(val), std::string>) {
          return val == "true" || val == "1";
        } else {
          return false;
        }
      },
      value_);
}

void Variable::fromString(const std::string &value) {
  if constexpr (std::is_same_v<decltype(value_), bool>) {
    value_ = (value == "true" || value == "1");
  } else if constexpr (std::is_same_v<decltype(value_), int>) {
    value_ = std::stoi(value);
  } else if constexpr (std::is_same_v<decltype(value_), float>) {
    value_ = std::stof(value);
  } else if constexpr (std::is_same_v<decltype(value_), std::string>) {
    value_ = value;
  }
}
