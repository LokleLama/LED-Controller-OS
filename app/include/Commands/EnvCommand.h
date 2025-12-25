#pragma once

#include "ICommand.h"
#include "IVariableStore.h"
#include "Console.h"
#include <string>

class EnvCommand : public ICommand {
public:
  EnvCommand(IVariableStore &store, const Console &console) : store(store), _console(console) {}

  // Returns the name of the command
  const std::string getName() const override { return "env"; }

  const std::string getHelp() const override {
    return "Usage: env          Lists all environment variables.\n"
           "       env save     saves the environment variables in an environment file called \"env\".\n"
           "       env load     loads the environment variables from an environment file called \"env\".";
  }

  int execute(const std::vector<std::string> &args) override {
    if (args.size() == 2 && args[1] == "save") {
      auto currentDirectory = _console.currentDirectory;
      if(!currentDirectory) {
        std::cout << "No filesystem loaded." << std::endl;
        return -1;
      }
      auto file = currentDirectory->openFile("env");
      if (!file) {
        file = currentDirectory->createFile("env");
        if (!file) {
          std::cout << "Failed to open or create 'env' file." << std::endl;
          return -1;
        }
      }
      if (store.saveToFile(file)) {
        std::cout << "Environment variables saved to 'env' file." << std::endl;
        return 0;
      }

      std::cout << "Failed to save environment variables to 'env' file." << std::endl;
      return -1;

    } else if (args.size() == 2 && args[1] == "load") {
      auto currentDirectory = _console.currentDirectory;
      if(!currentDirectory) {
        std::cout << "No filesystem loaded." << std::endl;
        return -1;
      }
      auto file = currentDirectory->openFile("env");
      if (!file) {
        std::cout << "Failed to open 'env' file." << std::endl;
        return -1;
      }
      if (store.loadFromFile(file)) {
        std::cout << "Environment variables loaded from 'env' file." << std::endl;
        return 0;
      }
      std::cout << "Failed to load environment variables from 'env' file." << std::endl;
      return -1;
    }

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
  const Console &_console; // Pointer to the console object
};
