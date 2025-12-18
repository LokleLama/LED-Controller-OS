#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ICommand.h"
#include "ITask.h"

#include "Flash/SPFS.h"

class Console : public ITask {
public:
  Console(std::shared_ptr<SPFS::Directory> rootDir);
  ~Console() {}

  bool registerCommand(std::shared_ptr<ICommand> command);

  std::vector<std::string> getCommandList() const;

  bool ExecuteTask() override;
  void Stop() { running = false; }

  std::shared_ptr<SPFS::Directory> currentDirectory;

private:
  std::vector<std::shared_ptr<ICommand>> commandList;
  bool running = true;

  std::shared_ptr<ICommand> findCommand(const std::string &name) const;
  void outputPrompt() const;
};