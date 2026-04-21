#include "VariableStore/VariableStore.h"
#include "VariableStore/BoolVariable.h"
#include "VariableStore/FloatVariable.h"
#include "VariableStore/IntVariable.h"
#include "VariableStore/StringVariable.h"
#include <cstddef>

#include "ArduinoJson.h"

#include "Mainloop.h"

VariableStore& VariableStore::getInstance() {
  static VariableStore instance;
  return instance;
}

std::shared_ptr<IVariable> VariableStore::findVariable(const std::string &key) const {
  for (const auto &var : _variables) {
    if (var->getName() == key) {
      return var;
    }
  }
  return nullptr;
}

bool VariableStore::setVariable(const std::string &key, const std::string &value) {
  if (_callbacks.find(key) != _callbacks.end()) {
    if (!_callbacks[key](key, value)) {
      return false;
    }
  }
  auto var = findVariable(key);
  _ignoreCallbacks = true;
  if (var && var->set(value)) {
    _ignoreCallbacks = false;
    return true;
  }
  _ignoreCallbacks = false;
  return false;
}
bool VariableStore::setBoolVariable(const std::string &key, bool value) {
  if (_callbacks.find(key) != _callbacks.end()) {
    if (!_callbacks[key](key, value ? "true" : "false")) {
      return false;
    }
  }
  auto var = findVariable(key);
  _ignoreCallbacks = true;
  if (var && var->setBool(value)) {
    _ignoreCallbacks = false;
    return true;
  }
  _ignoreCallbacks = false;
  return false;
}
bool VariableStore::setVariable(const std::string &key, float value) {
  if (_callbacks.find(key) != _callbacks.end()) {
    if (!_callbacks[key](key, std::to_string(value))) {
      return false;
    }
  }
  auto var = findVariable(key);
  _ignoreCallbacks = true;
  if (var && var->set(value)) {
    _ignoreCallbacks = false;
    return true;
  }
  _ignoreCallbacks = false;
  return false;
}
bool VariableStore::setVariable(const std::string &key, int value) {
  if (_callbacks.find(key) != _callbacks.end()) {
    if (!_callbacks[key](key, std::to_string(value))) {
      return false;
    }
  }
  auto var = findVariable(key);
  _ignoreCallbacks = true;
  if (var && var->set(value)) {
    _ignoreCallbacks = false;
    return true;
  }
  _ignoreCallbacks = false;
  return false;
}

bool VariableStore::setVariable(const std::string &key, unsigned int value) {
  return setVariable(key, static_cast<int>(value));
}

bool VariableStore::setVariable(const std::string &key, uint32_t value) {
  return setVariable(key, static_cast<int>(value));
}

std::shared_ptr<IVariable> VariableStore::addVariable(const std::string &key, 
                                                      const std::string &value) {
  auto var = findVariable(key);
  if(var) {
    setVariable(key, value);
    return var;
  }
  auto ret = std::make_shared<StringVariable>(key, value);
  _variables.push_back(ret);
  return ret;
}

std::shared_ptr<IVariable> VariableStore::addBoolVariable(const std::string &key,
                                                          bool value) {
  auto var = findVariable(key);
  if(var) {
    setBoolVariable(key, value);
    return var;
  }
  auto ret = std::make_shared<BoolVariable>(key, value);
  _variables.push_back(ret);
  return ret;
}

std::shared_ptr<IVariable> VariableStore::addVariable(const std::string &key,
                                                      float value) {
  auto var = findVariable(key);
  if(var) {
    setVariable(key, value);
    return var;
  }
  auto ret = std::make_shared<FloatVariable>(key, value);
  _variables.push_back(ret);
  return ret;
}

std::shared_ptr<IVariable> VariableStore::addVariable(const std::string &key,
                                                      int value) {
  auto var = findVariable(key);
  if(var){
    setVariable(key, value);
    return var;
  }
  auto ret = std::make_shared<IntVariable>(key, value);
  _variables.push_back(ret);
  return ret;
}

std::shared_ptr<IVariable> VariableStore::addVariable(const std::string &key,
                                                      unsigned int value) {
  return addVariable(key, static_cast<int>(value));
}

std::shared_ptr<IVariable> VariableStore::addVariable(const std::string &key,
                                                      uint32_t value) {
  return addVariable(key, static_cast<int>(value));
}

std::shared_ptr<IVariable> VariableStore::registerVariable(std::shared_ptr<IVariable> new_variable) {
  auto var = findVariable(new_variable->getName());
  if(var) {
    return var;
  }
  _variables.push_back(new_variable);
  return new_variable;
}

std::shared_ptr<IVariable> VariableStore::getVariable(const std::string &key) const {
  return findVariable(key);
}

size_t VariableStore::findVariableEnd(const std::string &input, size_t startPos) const {
  size_t pos = startPos;
  while (pos < input.length()) {
    char c = input[pos];
    if (std::iscntrl(c) || c == ' ') {
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
  auto var = findVariable(key);
  if (var) {
    var->setCallback([this](const std::string& varKey) {
      return this->valueChangedCallback(varKey);
    });
  }

  _callbacks[key] = callback;
}

Signal VariableStore::_signalNumber = '0';

Signal VariableStore::registerSignal(const std::string &key, Signal signal) {
  auto sig = _signals.find(key);
  if (sig != _signals.end()) {
    return sig->second;
  }

  if(signal == 0) {
    signal = 0x76617200 + _signalNumber;
    _signalNumber++;
    if((_signalNumber & 0xFF) == '0'){
      _signalNumber = 'A';
    }else if((_signalNumber & 0xFF) == 'Z' + 1){
      _signalNumber = 'a';
    }else if((_signalNumber & 0xFF) == 'z' + 1){
      _signalNumber = '0';
      _signalNumber += 0x100;
    }
  }
  _signals[key] = signal;
  return signal;
}

Signal VariableStore::getSignal(const std::string &key) const {
  auto sig = _signals.find(key);
  if (sig != _signals.end()) {
    return sig->second;
  }
  return 0;
}

bool VariableStore::valueChangedCallback(const std::string& key) {
  if (_signals.find(key) != _signals.end()) {
    Mainloop::getInstance().triggerSignal(_signals[key]);
  }

  if(_ignoreCallbacks) {
    return true;
  }

  if (_callbacks.find(key) != _callbacks.end()) {
    auto var = findVariable(key);
    if (var) {
      return _callbacks[key](key, var->asString());
    }
  }
  return true;
}

const std::unordered_map<std::string, std::string>
VariableStore::getAllVariables() const {
  std::unordered_map<std::string, std::string> result;

  for (const auto &var : _variables) {
    result[var->getName()] = var->asString();
  }
  return result;
}

bool VariableStore::saveToFile(std::shared_ptr<SPFS::File>& file) const {
  JsonDocument doc;
  for (const auto& var : _variables) {
    if (var->isSystemVariable()) {
      continue; // Skip system variables
    }
    switch (var->getType()) {
      case IVariable::Type::INT:
        doc[var->getName()] = var->asInt();
        break;
      case IVariable::Type::FLOAT:
        doc[var->getName()] = var->asFloat();
        break;
      case IVariable::Type::BOOL:
        doc[var->getName()] = var->asBool();
        break;
      default:
      case IVariable::Type::STRING:
        doc[var->getName()] = var->asString();
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
    auto var = findVariable(key);
    if (var == nullptr) {
      // Variable does not exist, create it based on the type in JSON
      if (kv.value().is<int>()) {
        auto ret = std::make_shared<IntVariable>(key, kv.value().as<int>());
        _variables.push_back(ret);
      } else if (kv.value().is<float>() || kv.value().is<double>()) {
        auto ret = std::make_shared<FloatVariable>(key, kv.value().as<float>());
        _variables.push_back(ret);
      } else if (kv.value().is<bool>()) {
        auto ret = std::make_shared<BoolVariable>(key, kv.value().as<bool>());
        _variables.push_back(ret);
      } else if (kv.value().is<std::string>()) {
        auto ret = std::make_shared<StringVariable>(key, kv.value().as<std::string>());
        _variables.push_back(ret);
      } else if (kv.value().is<const char*>()) {
        auto ret = std::make_shared<StringVariable>(key, std::string(kv.value().as<const char*>()));
        _variables.push_back(ret);
      }
    } else {
      // Variable exists, set its value
      var->set(kv.value().as<std::string>());
    }
  }
  return true;
}