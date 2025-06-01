#include "VariableStore/VariableStore.h"
#include "VariableStore/StringVariable.h"

bool VariableStore::setVariable(const std::string &key,
                                const std::string &value) {
  if (callbacks.find(key) != callbacks.end()) {
    if (!callbacks[key](key, value)) {
      return false;
    }
  }
  if (variables.find(key) == variables.end()) {
    variables[key] = std::make_shared<StringVariable>(value);
  } else {
    variables[key]->set(value);
  }
  return true;
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