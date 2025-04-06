#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ICommand.h"

class Console {
public:
  Console() = default;
  bool registerCommand(std::shared_ptr<ICommand> command);

  void consoleTask();

private:
  static const int INTERFACE_NUMBER = 0;
  std::vector<std::shared_ptr<ICommand>> commandList;
  bool isConnected = false;
  std::string inputBuffer;

  void OnNewConnection() const;
  std::shared_ptr<ICommand> findCommand(const std::string &name) const;
  void outputPrompt() const;
};
