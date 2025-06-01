#pragma once
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

#include "IVariable.h"
#include "IVariableStore.h"

class VariableStore : public IVariableStore {
public:
  bool setVariable(const std::string &key, const std::string &value) override;

  std::shared_ptr<IVariable> getVariable(const std::string &key) const override;

  void registerCallback(const std::string &key,
                        IVariableStore::Callback callback) override;

  const std::unordered_map<std::string, std::string>
  getAllVariables() const override;

private:
  std::unordered_map<std::string, std::shared_ptr<IVariable>> variables;
  std::unordered_map<std::string, IVariableStore::Callback> callbacks;
};