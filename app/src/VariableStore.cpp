#include "VariableStore.h"

bool VariableStore::setVariable(const std::string &key,
                                const std::string &value) {
  if (callbacks.find(key) != callbacks.end()) {
    if (!callbacks[key](key, value)) {
      return false;
    }
  }
  variables[key] = value;
  return true;
}

std::string VariableStore::getVariable(const std::string &key) const {
  auto it = variables.find(key);
  if (it != variables.end()) {
    return it->second;
  }
  return "";
}

void VariableStore::registerCallback(const std::string &key,
                                     IVariableStore::Callback callback) {
  callbacks[key] = callback;
}

const std::unordered_map<std::string, std::string> &
VariableStore::getAllVariables() const {
  return variables;
}