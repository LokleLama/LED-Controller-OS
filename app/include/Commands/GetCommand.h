#pragma once

#include "ICommand.h"
#include "IVariableStore.h"
#include <string>

class GetCommand : public ICommand {
public:
  GetCommand(IVariableStore &store) : store(store) {}

  // Returns the name of the command
  const std::string getName() const override { return "get"; }

  const std::string getHelp() const override {
    return "Usage: get <key>\n"
           "       Gets the value of the specified variable.";
  }

  int execute(const std::vector<std::string> &args) override {
    if (args.size() < 2) {
      std::cout << getHelp() << std::endl;
      return -1;
    }
    auto value = store.getVariable(args[1]);
    if (value.get() == nullptr) {
      std::cout << "Variable not found\n";
      return -1;
    }
    std::cout << args[1] << " = " << value->asString() << std::endl;
    return 0;
  }

private:
  IVariableStore &store;
};
