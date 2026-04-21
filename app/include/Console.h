#pragma once

#include <memory>
#include <string>
#include <vector>
#include <queue>

#include "ICommand.h"
#include "ITask.h"
#include "IVariableStore.h"

#include "Flash/SPFS.h"

class Console : public ITask {
public:
  Console(IVariableStore &variableStore, std::shared_ptr<SPFS> fs = nullptr);

  bool registerCommand(std::shared_ptr<ICommand> command);

  std::vector<std::string> getCommandList() const;
  std::shared_ptr<ICommand> findCommand(const std::string &name) const;

  bool ExecuteTask(TaskPID pid) override;

  const std::string getName() const override {
    return "Console Task";
  }

  bool EnqueueCommand(const std::string &command) {
    commandQueue.push(command);
    return true;
  }

  const std::shared_ptr<SPFS>& getFileSystem() const { return _fs; }

  std::shared_ptr<SPFS::Directory> currentDirectory;

  void setFileSystem(std::shared_ptr<SPFS> fs);

  TaskPID getPID() const {
    return static_cast<TaskPID>(pidVariable->asInt());
  }

private:
  IVariableStore &_variableStore;
  std::shared_ptr<SPFS> _fs;
  std::shared_ptr<IVariable> resultVariable;
  std::shared_ptr<IVariable> pidVariable;
  std::vector<std::shared_ptr<ICommand>> commandList;
  bool isConnected = false;
  std::string inputBuffer;
  char openingQuoteChar = '\0';
  std::queue<std::string> commandQueue;

  void OnNewConnection() const;
  void outputPrompt() const;

  bool ReadUART();
  bool ExecuteLine(const std::string &line);
};
