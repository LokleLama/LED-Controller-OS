#include "VariableStore/VariableStore.h"
#include "VariableStore/Variable.h" // Ensure this header defines the Variable class
#include <cstddef>
#include <unordered_map>

bool VariableStore::setVariable(const std::string &key,
                                const std::string &value) {
  if (callbacks.find(key) != callbacks.end()) {
    if (!callbacks[key](key, value)) {
      return false;
    }
  }
  if (variables.find(key) == variables.end()) {
    variables[key] = Variable(value);
  } else {
    variables[key].fromString(value);
  }
  return true;
}

std::string VariableStore::getVariable(const std::string &key) const {
  auto it = variables.find(key);
  if (it != variables.end()) {
    return it->second.toString();
  }
  return "";
}

void VariableStore::registerCallback(const std::string &key,
                                     IVariableStore::Callback callback) {
  callbacks[key] = callback;
}

const std::unordered_map<std::string, std::string> &
VariableStore::getAllVariables() const {
  return std::unordered_map<std::string, std::string>();
}