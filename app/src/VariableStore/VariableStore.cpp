#include "VariableStore/VariableStore.h"
#include "VariableStore/BoolVariable.h"
#include "VariableStore/FloatVariable.h"
#include "VariableStore/IntVariable.h"
#include "VariableStore/StringVariable.h"
#include <cstddef>

bool VariableStore::setVariable(const std::string &key,
                                const std::string &value) {
  if (callbacks.find(key) != callbacks.end()) {
    if (!callbacks[key](key, value)) {
      return false;
    }
  }
  if (variables.find(key) == variables.end()) {
    return false;
  }
  if (variables[key]->set(value)) {
    return true;
  }
  return false;
}
bool VariableStore::setBoolVariable(const std::string &key, bool value) {
  if (callbacks.find(key) != callbacks.end()) {
    if (!callbacks[key](key, value ? "true" : "false")) {
      return false;
    }
  }
  if (variables.find(key) == variables.end()) {
    return false;
  }
  if (variables[key]->set(value)) {
    return true;
  }
  return false;
}
bool VariableStore::setVariable(const std::string &key, float value) {
  if (callbacks.find(key) != callbacks.end()) {
    if (!callbacks[key](key, std::to_string(value))) {
      return false;
    }
  }
  if (variables.find(key) == variables.end()) {
    return false;
  }
  if (variables[key]->set(value)) {
    return true;
  }
  return false;
}
bool VariableStore::setVariable(const std::string &key, int value) {
  if (callbacks.find(key) != callbacks.end()) {
    if (!callbacks[key](key, std::to_string(value))) {
      return false;
    }
  }
  if (variables.find(key) == variables.end()) {
    return false;
  }
  if (variables[key]->set(value)) {
    return true;
  }
  return false;
}

std::shared_ptr<IVariable>
VariableStore::addVariable(const std::string &key, const std::string &value) {
  if (callbacks.find(key) != callbacks.end()) {
    if (!callbacks[key](key, value)) {
      return nullptr;
    }
  }
  if (variables.find(key) == variables.end()) {
    auto ret = std::make_shared<StringVariable>(value);
    variables[key] = ret;
    return ret;
  }
  if (variables[key]->set(value)) {
    return variables[key];
  }
  return nullptr;
}

std::shared_ptr<IVariable>
VariableStore::addBoolVariable(const std::string &key, bool value) {
  if (callbacks.find(key) != callbacks.end()) {
    if (!callbacks[key](key, value ? "true" : "false")) {
      return nullptr;
    }
  }
  if (variables.find(key) == variables.end()) {
    auto ret = std::make_shared<BoolVariable>(value);
    variables[key] = ret;
    return ret;
  }
  if (variables[key]->set(value)) {
    return variables[key];
  }
  return nullptr;
}

std::shared_ptr<IVariable> VariableStore::addVariable(const std::string &key,
                                                      float value) {
  if (callbacks.find(key) != callbacks.end()) {
    if (!callbacks[key](key, std::to_string(value))) {
      return nullptr;
    }
  }
  if (variables.find(key) == variables.end()) {
    auto ret = std::make_shared<FloatVariable>(value);
    variables[key] = ret;
    return ret;
  }
  if (variables[key]->set(value)) {
    return variables[key];
  }
  return nullptr;
}

std::shared_ptr<IVariable> VariableStore::addVariable(const std::string &key,
                                                      int value) {
  if (callbacks.find(key) != callbacks.end()) {
    if (!callbacks[key](key, std::to_string(value))) {
      return nullptr;
    }
  }
  if (variables.find(key) == variables.end()) {
    auto ret = std::make_shared<IntVariable>(value);
    variables[key] = ret;
    return ret;
  }
  if (variables[key]->set(value)) {
    return variables[key];
  }
  return nullptr;
}

std::shared_ptr<IVariable>
VariableStore::getVariable(const std::string &key) const {
  auto it = variables.find(key);
  if (it != variables.end()) {
    return it->second;
  }
  return nullptr;
}

void VariableStore::registerCallback(const std::string &key,
                                     IVariableStore::Callback callback) {
  callbacks[key] = callback;
}

const std::unordered_map<std::string, std::string>
VariableStore::getAllVariables() const {
  std::unordered_map<std::string, std::string> result;

  for (const auto &pair : variables) {
    result[pair.first] = pair.second->asString();
  }
  return result;
}