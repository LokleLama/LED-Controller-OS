#pragma once

#include <cstdint>
#include <string>

using TaskPID = int;

class ITask {
public:
  virtual ~ITask() = default;

  // when the task returns false, it will be removed from the main loop
  virtual bool ExecuteTask(TaskPID pid) = 0;

  virtual const std::string getName() const = 0;
};
