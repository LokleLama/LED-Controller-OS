#pragma once

#include <string>
#include <vector>

class ICommand {
public:
  virtual ~ICommand() = default;

  // Returns the name of the command
  virtual const std::string getName() const = 0;

  // Executes the command
  virtual int execute(const std::vector<std::string> &args) = 0;
};