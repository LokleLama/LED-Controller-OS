#pragma once
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

#include "IVariable.h"
#include "IVariableStore.h"

#include "Utils/Signal.h"

class VariableStore : public IVariableStore {
public:
  static VariableStore& getInstance();

  std::shared_ptr<IVariable> addVariable(const std::string &key,
                                         const std::string &value) override;
  std::shared_ptr<IVariable> addBoolVariable(const std::string &key,
                                             bool value) override;
  std::shared_ptr<IVariable> addVariable(const std::string &key,
                                         float value) override;
  std::shared_ptr<IVariable> addVariable(const std::string &key,
                                         int value) override;
  std::shared_ptr<IVariable> addVariable(const std::string &key,
                                         unsigned int value) override;
  std::shared_ptr<IVariable> addVariable(const std::string &key,
                                         uint32_t value) override;

  std::shared_ptr<IVariable> registerVariable(std::shared_ptr<IVariable> new_variable) override;

  bool setVariable(const std::string &key, const std::string &value) override;
  bool setBoolVariable(const std::string &key, bool value) override;
  bool setVariable(const std::string &key, float value) override;
  bool setVariable(const std::string &key, int value) override;
  bool setVariable(const std::string &key, unsigned int value) override;
  bool setVariable(const std::string &key, uint32_t value) override;

  std::shared_ptr<IVariable> getVariable(const std::string &key) const override;
  
  std::string findAndReplaceVariables(const std::string &input) const override;

  void registerCallback(const std::string &key,
                        IVariableStore::Callback callback) override;

  Signal registerSignal(const std::string &key, Signal signal = 0) override;

  const std::unordered_map<std::string, std::string>
  getAllVariables() const override;

  bool saveToFile(std::shared_ptr<SPFS::File>& file) const override;
  bool loadFromFile(const std::shared_ptr<SPFS::File>& file) override;

private:
  std::vector<std::shared_ptr<IVariable>> _variables;
  std::unordered_map<std::string, IVariableStore::Callback> _callbacks;
  std::unordered_map<std::string, Signal> _signals;
  bool _ignoreCallbacks = false;

  static Signal _signalNumber;

  VariableStore() = default;
  
  size_t findVariableEnd(const std::string &input, size_t startPos) const;
  bool valueChangedCallback(const std::string& key);

  std::shared_ptr<IVariable> findVariable(const std::string &key) const;
};