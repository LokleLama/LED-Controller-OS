#pragma once

#include "ICommand.h"
#include "IVariableStore.h"
#include <string>

class SetCommand : public ICommand {
public:
  SetCommand(IVariableStore &store) : store(store) {}

  // Returns the name of the command
  const std::string getName() const override { return "set"; }

  int execute(const std::vector<std::string> &args) override {
    if (args.size() < 3) {
      std::cout << "Usage: " << args[0] << " [-a] <key> <value>\n";
      std::cout << "  -a: Set variable and create it if it doesn't exist\n";
      return -1;
    }
    if (args[1] == "-a") {
      if (args.size() < 4) {
        std::cout << "Usage: " << args[0] << " -a <key> <value>\n";
        return -1;
      }
      if (store.addVariable(args[2], args[3]) != nullptr) {
        std::cout << "Variable \"" << args[2] << "\" set to \"" << args[3]
                  << "\"" << std::endl;
        return 0;
      }
      std::cout << "Failed to add variable \"" << args[2] << "\"" << std::endl;
      return -1;
    }
    if (store.setVariable(args[1], args[2])) {
      std::cout << "Variable \"" << args[1] << "\" set to \"" << args[2] << "\""
                << std::endl;
      return 0;
    }
    std::cout << "Failed to set variable \"" << args[1] << "\"" << std::endl;
    return -1;
  }

private:
  IVariableStore &store;
};
