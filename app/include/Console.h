#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ICommand.h"
#include "IVariableStore.h"

class Console {
public:
  Console(IVariableStore &variableStore) : variableStore(variableStore) {}
  ~Console(){};

  bool registerCommand(std::shared_ptr<ICommand> command);

  void consoleTask();

private:
  static const int INTERFACE_NUMBER = 0;
  std::vector<std::shared_ptr<ICommand>> commandList;
  bool isConnected = false;
  std::string inputBuffer;
  IVariableStore &variableStore;

  void OnNewConnection() const;
  std::shared_ptr<ICommand> findCommand(const std::string &name) const;
  void outputPrompt() const;
};
