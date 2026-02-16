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
  void registerRegularTask(const std::string &name, Function func) { _regularTasks.push_back(new RegularTask(name, func)); }

  void registerRegularTask(ITask *task) {
    _regularTasks.push_back(task);
  }

  void registerTimedTask(ITask *task, int32_t intervalMs) {
    _timedTasks.push_back({task->getName(), [task]() { return task->ExecuteTask(); }, false, intervalMs, _systickCounter + intervalMs});
  }

  // Register a function to be executed every x ms
  void registerTimedTask(const std::string &name, Function func, int32_t intervalMs) {
    _timedTasks.push_back({name, func, false, intervalMs, _systickCounter + intervalMs});
  }

  // Register a function to be executed once after y ms
  void registerDelayedTask(const std::string &name, Function func, int32_t delayMs) {
    _timedTasks.push_back({name, func, false, -1, _systickCounter + delayMs});
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

  Mainloop();

  void onMillisecond();

  static bool alarm_callback(repeating_timer *rt);
};