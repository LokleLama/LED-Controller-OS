#include "VariableStore/VariableStore.h"
#include "VariableStore/BoolVariable.h"
#include "VariableStore/FloatVariable.h"
#include "VariableStore/IntVariable.h"
#include "VariableStore/StringVariable.h"
#include <cstddef>

#include "ArduinoJson.h"

VariableStore& VariableStore::getInstance() {
  static VariableStore instance;
  return instance;
}

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

size_t VariableStore::findVariableEnd(const std::string &input,
                                      size_t startPos) const {
  size_t pos = startPos;
  while (pos < input.length()) {
    char c = input[pos];
    if (std::iscntrl(c) || c == ' ' || c == '$') {
      break;
    }
    pos++;
  }
  return pos;
}

std::string VariableStore::findAndReplaceVariables(const std::string &input) const {
  std::string result = input;
  size_t pos = 0;
  while ((pos = result.find("${", pos)) != std::string::npos) {
    if (pos > 0 && result[pos - 1] == '\\') {
      result.erase(pos - 1, 1); // Remove escape character
      pos += 1; // Move past the escaped variable
      continue;
    }
    size_t endPos = result.find("}", pos);
    if (endPos == std::string::npos) {
      break; // No closing parenthesis found
    }
    std::string varName = result.substr(pos + 2, endPos - pos - 2);
    auto var = getVariable(varName);
    if(var){
      std::string varValue = "\"" + var->asValueString() + "\"";
      result.replace(pos, endPos - pos + 1, varValue);
      pos += varValue.length(); // Move past the replaced value
    }else{
      pos = endPos + 1; // Move past the closing brace if variable not found
    }
  }
  pos = 0;
  while ((pos = result.find("$", pos)) != std::string::npos) {
    if (pos > 0 && result[pos - 1] == '\\') {
      result.erase(pos - 1, 1); // Remove escape character
      pos += 1; // Move past the escaped variable
      continue;
    }
    size_t endPos = findVariableEnd(result, pos + 1);
    std::string varName = result.substr(pos + 1, endPos - pos - 1);
    auto var = getVariable(varName);
    if(var){
      std::string varValue = var->asValueString();
      result.replace(pos, endPos - pos, varValue);
      pos += varValue.length(); // Move past the replaced value
    }else{
      pos = endPos + 1; // Move past the closing brace if variable not found
    }
  }
  return result;
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
    if (pair.first == "?" || pair.first.substr(0, 4) == "var.") {
      continue; // Skip internal variables
    }
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
  if(file == nullptr) {
    return false;
  }

  std::string fileContent = file->readAsString();
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, fileContent);
  if (error) {
    return false;
  }

  for (JsonPair kv : doc.as<JsonObject>()) {
    const std::string key = kv.key().c_str();
    if (variables.find(key) == variables.end()) {
      // Variable does not exist, create it based on the type in JSON
      if (kv.value().is<int>()) {
        auto ret = std::make_shared<IntVariable>(kv.value().as<int>());
        variables[key] = ret;
      } else if (kv.value().is<float>() || kv.value().is<double>()) {
        auto ret = std::make_shared<FloatVariable>(kv.value().as<float>());
        variables[key] = ret;
      } else if (kv.value().is<bool>()) {
        auto ret = std::make_shared<BoolVariable>(kv.value().as<bool>());
        variables[key] = ret;
      } else if (kv.value().is<std::string>()) {
        auto ret = std::make_shared<StringVariable>(kv.value().as<std::string>());
        variables[key] = ret;
      } else if (kv.value().is<const char*>()) {
        auto ret = std::make_shared<StringVariable>(std::string(kv.value().as<const char*>()));
        variables[key] = ret;
      }
    } else {
      // Variable exists, set its value
      variables[key]->set(kv.value().as<std::string>());
    }
  }
  return true;
}