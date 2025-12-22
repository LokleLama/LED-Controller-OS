#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ICommand.h"
#include "ITask.h"
#include "IVariableStore.h"

#include "Flash/SPFS.h"

class Console : public ITask {
public:
  Console(IVariableStore &variableStore, std::shared_ptr<SPFS> fs = nullptr) : 
    _variableStore(variableStore), _fs(fs) 
  {
    if(_fs != nullptr) {
      currentDirectory = _fs->getRootDirectory();
    } else {
      currentDirectory = nullptr;
    }
  }
  ~Console(){};

  bool registerCommand(std::shared_ptr<ICommand> command);

  std::vector<std::string> getCommandList() const;
  std::shared_ptr<ICommand> findCommand(const std::string &name) const;

  bool ExecuteTask() override;

  const std::shared_ptr<SPFS>& getFileSystem() const { return _fs; }

  std::shared_ptr<SPFS::Directory> currentDirectory;

private:
  IVariableStore &_variableStore;
  std::shared_ptr<SPFS> _fs;
  std::vector<std::shared_ptr<ICommand>> commandList;
  bool isConnected = false;
  std::string inputBuffer;

  void OnNewConnection() const;
  void outputPrompt() const;
};
