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

  virtual std::shared_ptr<IVariable> addVariable(const std::string &key,
                                                 const std::string &value) = 0;
  virtual std::shared_ptr<IVariable> addBoolVariable(const std::string &key,
                                                     bool value) = 0;
  virtual std::shared_ptr<IVariable> addVariable(const std::string &key,
                                                 int value) = 0;
  virtual std::shared_ptr<IVariable> addVariable(const std::string &key,
                                                 float value) = 0;

  virtual bool setVariable(const std::string &key,
                           const std::string &value) = 0;
  virtual bool setBoolVariable(const std::string &key, bool value) = 0;
  virtual bool setVariable(const std::string &key, int value) = 0;
  virtual bool setVariable(const std::string &key, float value) = 0;

  virtual std::shared_ptr<IVariable>
  getVariable(const std::string &key) const = 0;

  virtual void registerCallback(const std::string &key, Callback callback) = 0;

  virtual const std::unordered_map<std::string, std::string>
  getAllVariables() const = 0;
};