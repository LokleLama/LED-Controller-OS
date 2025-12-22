#pragma once

#include "ICommand.h"
#include "IVariableStore.h"
#include <string>

class EnvCommand : public ICommand {
public:
  EnvCommand(IVariableStore &store) : store(store) {}

  // Returns the name of the command
  const std::string getName() const override { return "env"; }

  const std::string getHelp() const override {
    return "Usage: env\n"
           "       Lists all environment variables.";
  }

  int execute(const std::vector<std::string> &args) override {
    const auto value = store.getAllVariables();
    if (value.empty()) {
      std::cout << "No variables found\n";
      return -1;
    }
    std::cout << "Variables:\n";
    for (const auto &pair : value) {
      std::cout << pair.first << " = " << pair.second << std::endl;
    }
    return 0;
  }

private:
  IVariableStore &store;
};
