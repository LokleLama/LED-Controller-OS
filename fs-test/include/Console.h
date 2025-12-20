#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ICommand.h"
#include "ITask.h"

#include "Flash/SPFS.h"

class Console : public ITask {
public:
  Console(std::shared_ptr<SPFS> fs);
  ~Console() {}

  bool registerCommand(std::shared_ptr<ICommand> command);

  std::vector<std::string> getCommandList() const;

  bool ExecuteTask() override;
  void Stop() { running = false; }

  const std::shared_ptr<SPFS>& getFileSystem() const { return _fs; }

  std::shared_ptr<SPFS::Directory> currentDirectory;

private:
  std::shared_ptr<SPFS> _fs;
  std::vector<std::shared_ptr<ICommand>> commandList;
  bool running = true;

  std::shared_ptr<ICommand> findCommand(const std::string &name) const;
  void outputPrompt() const;
};