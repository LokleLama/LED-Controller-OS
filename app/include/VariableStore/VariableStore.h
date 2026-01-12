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
  std::shared_ptr<IVariable> addVariable(const std::string &key,
                                         const std::string &value) override;
  std::shared_ptr<IVariable> addBoolVariable(const std::string &key,
                                             bool value) override;
  std::shared_ptr<IVariable> addVariable(const std::string &key,
                                         int value) override;
  std::shared_ptr<IVariable> addVariable(const std::string &key,
                                         float value) override;

  bool setVariable(const std::string &key, const std::string &value) override;
  bool setBoolVariable(const std::string &key, bool value) override;
  bool setVariable(const std::string &key, float value) override;
  bool setVariable(const std::string &key, int value) override;

  std::shared_ptr<IVariable> getVariable(const std::string &key) const override;
  
  std::string findAndReplaceVariables(const std::string &input) const override;

  void registerCallback(const std::string &key,
                        IVariableStore::Callback callback) override;

  const std::unordered_map<std::string, std::string>
  getAllVariables() const override;

  bool saveToFile(std::shared_ptr<SPFS::File>& file) const override;
  bool loadFromFile(const std::shared_ptr<SPFS::File>& file) override;

private:
  std::unordered_map<std::string, std::shared_ptr<IVariable>> variables;
  std::unordered_map<std::string, IVariableStore::Callback> callbacks;
  
  size_t findVariableEnd(const std::string &input, size_t startPos) const;
};