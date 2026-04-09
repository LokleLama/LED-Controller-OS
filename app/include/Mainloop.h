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
  struct TaskInfo {
    TaskHandle handle;
    std::string name;
    Function func;

    uint64_t startTime;
    uint32_t meanTime;
    uint32_t maxTime;
  };

  struct TimedTaskInfo {
    struct TaskInfo info;

    bool execute;
    int32_t intervalMs;
    uint32_t nextExecution;
  };

  struct SignalTaskInfo {
    struct TaskInfo info;

    uint32_t signal;
    bool execute;
  };

public:
  static Mainloop& getInstance();

  // Register a function to be executed in every mainloop iteration
  TaskHandle registerRegularTask(const std::string &name, Function func) {
    TaskHandle handle = _nextTaskHandle++;
    _regularTasks.push_back({handle, name, func, 0, 0, 0});
    return handle;
  }

  TaskHandle registerRegularTask(ITask *task) {
    return registerRegularTask(task->getName(), [task]() { return task->ExecuteTask(); });
  }

  TaskHandle registerTimedTask(ITask *task, int32_t intervalMs, int32_t initialDelayMs = 0) {
    return registerTimedTask(task->getName(), [task]() { return task->ExecuteTask(); }, intervalMs, initialDelayMs);
  }

  // Register a function to be executed every x ms
  TaskHandle registerTimedTask(const std::string &name, Function func, int32_t intervalMs, int32_t initialDelayMs = 0) {
    TaskHandle handle = _nextTaskHandle++;
    TimedTaskInfo taskInfo{{handle, name, func, 0, 0, 0}, initialDelayMs == 0, intervalMs, _systickCounter + initialDelayMs};
    if(initialDelayMs == 0){
      taskInfo.nextExecution += intervalMs;
    }
    _timedTasks.push_back(taskInfo);
    return handle;
  }

  // Register a function to be executed once after y ms
  TaskHandle registerDelayedTask(const std::string &name, Function func, int32_t delayMs) {
    return registerTimedTask(name, func, -1, delayMs);
  }

  bool modifyTimedTaskInterval(TaskHandle handle, int32_t newIntervalMs) {
    for (auto &task : _timedTasks) {
      if (task.info.handle == handle) {
        int32_t difference = newIntervalMs - task.intervalMs;
        task.nextExecution = task.nextExecution + difference;
        task.intervalMs = newIntervalMs;
        if (task.nextExecution <= _systickCounter) {
          task.nextExecution = _systickCounter + newIntervalMs;
          task.execute = true;
        }
        
        return true;
      }
    }
    return false;
  }

  TaskHandle registerSignalTask(ITask *task, uint32_t signal) {
    return registerSignalTask(task->getName(), [task]() { return task->ExecuteTask(); }, signal);
  }

  TaskHandle registerSignalTask(const std::string &name, Function func, uint32_t signal) {
    TaskHandle handle = _nextTaskHandle++;
    _signalTasks.push_back({{handle, name, func, 0, 0, 0}, signal, false});
    return handle;
  }

  void triggerSignal(uint32_t signal) {
    for (auto &task : _signalTasks) {
      if (task.signal == signal) {
        task.execute = true;
      }
    }
  }

  // Start the mainloop
  void start();

  // Stop the mainloop
  void stop() { _running = false; }

  // Simulated SysTick functions
  uint32_t getSysTick() { return _systickCounter; }

  void OuptutTaskInformation();

private:
  std::vector<TaskInfo> _regularTasks;
  std::vector<TimedTaskInfo> _timedTasks;
  std::vector<SignalTaskInfo> _signalTasks;

  uint32_t _loop_statistic[8];
  uint32_t _max_loop_time;
  int _loop_statistic_ptr;

  bool _running;
  uint32_t _systickCounter = 0;
  TaskHandle _nextTaskHandle = 0;

  Mainloop();

  void onMillisecond();
  void calculateStatistics(struct TaskInfo &task);
  void OuptutTaskInformation(const struct TaskInfo &task);

  static bool alarm_callback(repeating_timer *rt);
};