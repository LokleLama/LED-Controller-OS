#pragma once

#include "pico/stdlib.h"
#include <cstdint>
#include <functional>
#include <vector>

#include "ITask.h"

class Mainloop {
public:
  using Function = std::function<bool()>;

  Mainloop();

  // Register a function to be executed in every mainloop iteration
  void registerRegularTask(Function func) { _regularTasks.push_back(func); }

  void registerRegularTask(ITask *task) {
    _regularTasks.push_back([task]() { return task->ExecuteTask(); });
  }

  // Register a function to be executed every x ms
  void registerTimedTask(Function func, int32_t intervalMs) {
    _timedTasks.push_back(
        {func, false, intervalMs, _systickCounter + intervalMs});
  }

  // Register a function to be executed once after y ms
  void registerDelayedTask(Function func, int32_t delayMs) {
    _delayedTasks.push_back({func, false, 1, _systickCounter + delayMs});
  }

  // Start the mainloop
  void start();

  // Stop the mainloop
  void stop() { _running = false; }

  // Simulated SysTick functions
  uint32_t getSysTick() { return _systickCounter; }

private:
  struct TimedTaskInfo {
    Function func;
    bool execute;
    int32_t intervalMs;
    uint32_t nextExecution;
  };
  struct DelayedTaskInfo {
    Function func;
    bool execute;
    int executionCounter;
    uint32_t nextExecution;
  };

  std::vector<Function> _regularTasks;
  std::vector<TimedTaskInfo> _timedTasks;
  std::vector<DelayedTaskInfo> _delayedTasks;

  bool _running;
  uint32_t _systickCounter;

  void onMillisecond();

  static bool alarm_callback(repeating_timer *rt);
};