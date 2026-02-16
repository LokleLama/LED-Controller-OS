#pragma once

#include <cstdint>
#include <string>

class ITask {
public:
  virtual ~ITask() = default;

  // when the task returns false, it will be removed from the main loop
  virtual bool ExecuteTask() = 0;

  virtual std::string getName() const = 0;
};
