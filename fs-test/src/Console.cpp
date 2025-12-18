#include "Console.h"
#include <cstddef>
#include <iostream>
#include <sstream>

#include "Commands/ExitCommand.h"
#include "Commands/HelpCommand.h"
#include "Commands/DirCommand.h"
#include "Commands/ChangeDirCommand.h"
#include "Commands/MakeDirCommand.h"
#include "Commands/CatCommand.h"

Console::Console(std::shared_ptr<SPFS::Directory> rootDir)  : currentDirectory(rootDir) {
    // You can register default commands here if needed
    registerCommand(std::make_shared<ExitCommand>(*this));
    registerCommand(std::make_shared<HelpCommand>(*this));
    registerCommand(std::make_shared<DirCommand>(*this));
    registerCommand(std::make_shared<ChangeDirCommand>(*this));
    registerCommand(std::make_shared<MakeDirCommand>(*this));
    registerCommand(std::make_shared<CatCommand>(*this));
}

bool Console::registerCommand(std::shared_ptr<ICommand> command) {
  if (!command) {
    return false;
  }
  commandList.push_back(command);
  return true;
}

std::shared_ptr<ICommand> Console::findCommand(const std::string &name) const {
  for (const auto &cmd : commandList) {
    if (cmd->getName() == name) {
      return cmd;
    }
  }
  return nullptr;
}

std::vector<std::string> Console::getCommandList() const {
  std::vector<std::string> commandNames;
  for (const auto &cmd : commandList) {
    commandNames.push_back(cmd->getName());
  }
  return commandNames;
}

bool Console::ExecuteTask() {
    while (running){
        outputPrompt();


        std::string input;
        std::getline(std::cin, input);
        std::istringstream stream(input);
        std::vector<std::string> tokens;
        std::string token;
        while (stream >> token) {
            tokens.push_back(token);
        }

        std::string command = tokens.empty() ? "" : tokens[0];
        auto cmd = findCommand(command);
        if (cmd != nullptr) {
            cmd->execute(tokens);
        } else {
            std::cout << "Unknown command: " << command << std::endl;
        }
    }

  return true;
}

void Console::outputPrompt() const {
  std::cout << currentDirectory->getFullPath() << " > ";
  std::cout.flush();
}