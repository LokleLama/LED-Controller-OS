#pragma once

#include "ICommand.h"
#include "IVariableStore.h"
#include "VariableStore/SpecialVariables/LinearTransformation.h"
#include <string>

class CreateVarCommand : public ICommand {
public:
  CreateVarCommand(IVariableStore &store) : store(store) {}

  // Returns the name of the command
  const std::string getName() const override { return "create"; }

  const std::string getHelp() const override {
    return "Usage: create <Name> <Type> <parameter...>\n"
           "       Type       | parameter...\n"
           "       -----------|---------------------------------\n"
           "       linear     | [multiplier] [offset]";
  }

  int execute(const std::vector<std::string> &args) override {
    if (args.size() < 3) {
      std::cout << getHelp() << std::endl;
      return -1;
    }

    const std::string& name = args[1];

    if (args[2] == "linear") {
        float multiplier = 1.0f;
        float offset = 0.0f;

        if (args.size() >= 4) {
            multiplier = std::stof(args[3]);
        }
        if (args.size() >= 5) {
            offset = std::stof(args[4]);
        }
        if (store.registerVariable(std::make_shared<LinearTransformation>(name, multiplier, offset)) != nullptr) {
            std::cout << "Variable \"" << name << "\" created with linear transformation (multiplier: " << multiplier << ", offset: " << offset << ")" << std::endl;
            return 0;
        }
    }

    std::cout << "Failed to create variable \"" << name << "\"" << std::endl;
    std::cout << getHelp() << std::endl;

    return -1;
  }

private:
  IVariableStore &store;
};
