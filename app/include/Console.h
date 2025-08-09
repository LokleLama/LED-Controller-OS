#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ICommand.h"
#include "ITask.h"
#include "IVariableStore.h"

class Console : public ITask {
public:
  Console(IVariableStore &variableStore) : variableStore(variableStore) {}
  ~Console(){};

  bool registerCommand(std::shared_ptr<ICommand> command);

  std::vector<std::string> getCommandList() const;

  bool ExecuteTask() override;

private:
  std::vector<std::shared_ptr<ICommand>> commandList;
  bool isConnected = false;
  std::string inputBuffer;
  IVariableStore &variableStore;

  void OnNewConnection() const;
  std::shared_ptr<ICommand> findCommand(const std::string &name) const;
  void outputPrompt() const;
};
