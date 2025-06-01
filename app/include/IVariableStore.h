#pragma once
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "IVariable.h"

class IVariableStore {
public:
  using Callback =
      std::function<bool(const std::string &, const std::string &)>;

  virtual ~IVariableStore() = default;

  virtual bool setVariable(const std::string &key,
                           const std::string &value) = 0;
  virtual std::shared_ptr<IVariable>
  getVariable(const std::string &key) const = 0;
  virtual void registerCallback(const std::string &key, Callback callback) = 0;
  virtual const std::unordered_map<std::string, std::string>
  getAllVariables() const = 0;
};