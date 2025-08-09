#pragma once

#include <cstdint>

class ITask {
public:
  virtual ~ITask() = default;

  virtual bool ExecuteTask() = 0;
};
