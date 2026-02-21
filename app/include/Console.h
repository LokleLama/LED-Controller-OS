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
  Console(IVariableStore &variableStore, std::shared_ptr<SPFS> fs = nullptr) : 
    _variableStore(variableStore)
  {
    setFileSystem(fs);
    resultVariable = variableStore.getVariable("?");
  }
  ~Console(){};

  bool registerCommand(std::shared_ptr<ICommand> command);

  std::vector<std::string> getCommandList() const;
  std::shared_ptr<ICommand> findCommand(const std::string &name) const;

  bool ExecuteTask() override;

  std::string getName() const override {
    return "Console Task";
  }

  bool EnqueueCommand(const std::string &command) {
    commandQueue.push(command);
    return true;
  }

  const std::shared_ptr<SPFS>& getFileSystem() const { return _fs; }

  std::shared_ptr<SPFS::Directory> currentDirectory;

  void setFileSystem(std::shared_ptr<SPFS> fs) {
    _fs = fs;
    if(_fs != nullptr) {
      currentDirectory = _fs->getRootDirectory();
    } else {
      currentDirectory = nullptr;
    }
  }

private:
  IVariableStore &_variableStore;
  std::shared_ptr<SPFS> _fs;
  std::shared_ptr<IVariable> resultVariable;
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
