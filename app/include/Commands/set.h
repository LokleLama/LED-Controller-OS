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
      std::cout << "Usage: set <key> <value>\n";
      return -1;
    }
    store.setVariable(args[1], args[2]);
    std::cout << "Set " << args[1] << " to " << args[2] << std::endl;
    return 0;
  }

private:
  IVariableStore &store;
};
