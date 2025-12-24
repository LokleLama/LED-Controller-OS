#include "VariableStore/VariableStore.h"
#include "VariableStore/BoolVariable.h"
#include "VariableStore/FloatVariable.h"
#include "VariableStore/IntVariable.h"
#include "VariableStore/StringVariable.h"
#include <cstddef>

#include "ArduinoJson.h"

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

bool VariableStore::saveToFile(std::shared_ptr<SPFS::File>& file) const {
  JsonDocument doc;
  for (const auto& pair : variables) {
    switch (pair.second->getType()) {
      case IVariable::Type::INT:
        doc[pair.first] = pair.second->asInt();
        break;
      case IVariable::Type::FLOAT:
        doc[pair.first] = pair.second->asFloat();
        break;
      case IVariable::Type::BOOL:
        doc[pair.first] = pair.second->asBool();
        break;
      default:
      case IVariable::Type::STRING:
        doc[pair.first] = pair.second->asString();
        break;
    }
  }

  std::string jsonString;
  if (serializeJson(doc, jsonString) == 0) {
    return false;
  }

  return file->write(jsonString);
}

bool VariableStore::loadFromFile(const std::shared_ptr<SPFS::File>& file) {
  
  /*DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, *file);
  if (error) {
    return false;
  }
  for (JsonPair kv : doc.as<JsonObject>()) {
    const std::string key = kv.key().c_str();
    const std::string value = kv.value().as<std::string>();
    if (variables.find(key) == variables.end()) {
      auto ret = std::make_shared<StringVariable>(value);
      variables[key] = ret;
    } else {
      variables[key]->set(value);
    }
  }*/
  return true;
}