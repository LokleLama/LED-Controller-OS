#pragma once

#include "pico/stdlib.h"
#include <cstdint>
#include <functional>
#include <vector>
#include <iostream>

#include "ITask.h"

class Mainloop {
public:
  using Function = std::function<bool()>;
  using TaskHandle = int;

private:
  class RegularTask : public ITask {
  public:
    RegularTask(const std::string &name, Function func) : _name(name), _func(func) {}
    bool ExecuteTask() override { return _func(); }
    std::string getName() const override { return _name; }
  private:
    Function _func;
    std::string _name;
  };

public:
  static Mainloop& getInstance();

  // Register a function to be executed in every mainloop iteration
  TaskHandle registerRegularTask(const std::string &name, Function func) {
    _regularTasks.push_back(new RegularTask(name, func));
    return _nextTaskHandle++;
  }

  TaskHandle registerRegularTask(ITask *task) {
    _regularTasks.push_back(task);
    return _nextTaskHandle++;
  }

  TaskHandle registerTimedTask(ITask *task, int32_t intervalMs) {
    TaskHandle handle = _nextTaskHandle++;
    _timedTasks.push_back({handle, task->getName(), [task]() { return task->ExecuteTask(); }, false, intervalMs, _systickCounter + intervalMs});
    return handle;
  }

  // Register a function to be executed every x ms
  TaskHandle registerTimedTask(const std::string &name, Function func, int32_t intervalMs) {
    TaskHandle handle = _nextTaskHandle++;
    _timedTasks.push_back({handle, name, func, false, intervalMs, _systickCounter + intervalMs});
    return handle;
  }

  // Register a function to be executed once after y ms
  TaskHandle registerDelayedTask(const std::string &name, Function func, int32_t delayMs) {
    TaskHandle handle = _nextTaskHandle++;
    _timedTasks.push_back({handle, name, func, false, -1, _systickCounter + delayMs});
    return handle;
  }

  bool modifyTimedTaskInterval(TaskHandle handle, int32_t newIntervalMs) {
    for (auto &task : _timedTasks) {
      if (task.handle == handle) {
        task.nextExecution = task.nextExecution - task.intervalMs + newIntervalMs;
        task.intervalMs = newIntervalMs;
        return true;
      }
    }
    return false;
  }

  // Start the mainloop
  void start();

  // Stop the mainloop
  void stop() { _running = false; }

  // Simulated SysTick functions
  uint32_t getSysTick() { return _systickCounter; }

  void OuptutTaskInformation() {
    std::cout << "Regular Tasks:" << std::endl;
    for (auto task : _regularTasks) {
      std::cout << "  - " << task->getName() << std::endl;
    }

    std::cout << "Timed Tasks:" << std::endl;
    for (auto &task : _timedTasks) {
      std::cout << "  - " << task.name << " (next execution in " << (task.nextExecution - _systickCounter) << " ms)" << std::endl;
    }
  }

private:
  struct TimedTaskInfo {
    TaskHandle handle;
    std::string name;
    Function func;
    bool execute;
    int32_t intervalMs;
    uint32_t nextExecution;
  };

  std::vector<ITask*> _regularTasks;
  std::vector<TimedTaskInfo> _timedTasks;

  bool _running;
  uint32_t _systickCounter;
  TaskHandle _nextTaskHandle = 0;

  Mainloop();

  void onMillisecond();

  static bool alarm_callback(repeating_timer *rt);
};