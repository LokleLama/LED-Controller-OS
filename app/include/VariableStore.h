#pragma once
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>

#include "IVariableStore.h"

class VariableStore : public IVariableStore {
public:
  void setVariable(const std::string &key, const std::string &value) override {
    variables[key] = value;
    if (callbacks.find(key) != callbacks.end()) {
      callbacks[key](key, value);
    }
  }

  std::string getVariable(const std::string &key) const override {
    auto it = variables.find(key);
    if (it != variables.end()) {
      return it->second;
    }
    return "";
  }

  void registerCallback(const std::string &key,
                        IVariableStore::Callback callback) override {
    callbacks[key] = callback;
  }

  const std::unordered_map<std::string, std::string> &
  getAllVariables() const override {
    return variables;
  }

private:
  std::unordered_map<std::string, std::string> variables;
  std::unordered_map<std::string, IVariableStore::Callback> callbacks;
};