#pragma once

#include <cstdint>

class ITask {
public:
  virtual ~ITask() = default;

  // when the task returns false, it will be removed from the main loop
  virtual bool ExecuteTask() = 0;
};
