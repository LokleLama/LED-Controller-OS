#pragma once

#include "pico/stdlib.h"
#include <cstdint>
#include <functional>
#include <vector>
#include <iostream>

#include "ITask.h"
#include "Utils/Signal.h"

class Mainloop {
public:
  using Function = std::function<bool(TaskPID)>;

private:
  struct TaskInfo {
    TaskPID pid;
    std::string name;
    Function func;

    uint64_t startTime;
    uint32_t meanTime;
    uint32_t maxTime;
  };

  struct RegularTaskInfo {
    struct TaskInfo info;

    uint32_t sleepUntil;
  };

  struct TimedTaskInfo {
    struct TaskInfo info;

    bool execute;
    int32_t intervalMs;
    uint32_t nextExecution;
  };

  struct SignalTaskInfo {
    struct TaskInfo info;

    SignalFilter filter;
    bool execute;
  };

public:
  static Mainloop& getInstance();

  // Register a function to be executed in every mainloop iteration
  TaskPID registerRegularTask(const std::string &name, Function func) {
    TaskPID handle = _nextTaskPID++;
    _regularTasks.push_back({{handle, name, func, 0, 0, 0}, 0});
    return handle;
  }

  TaskPID registerRegularTask(ITask *task) {
    return registerRegularTask(task->getName(), [task](TaskPID pid) { return task->ExecuteTask(pid); });
  }

  bool sleepTask(TaskPID handle, uint32_t sleepTimeMs) {
    for (auto &task : _regularTasks) {
      if (task.info.pid == handle) {
        task.sleepUntil = _systickCounter + sleepTimeMs;
        return true;
      }
    }
    for (auto &task : _timedTasks) {
      if (task.info.pid == handle) {
        task.nextExecution = _systickCounter + sleepTimeMs;
        task.execute = false;
        return true;
      }
    }
    return false;
  }

  TaskPID registerTimedTask(ITask *task, int32_t intervalMs, int32_t initialDelayMs = 0) {
    return registerTimedTask(task->getName(), [task](TaskPID pid) { return task->ExecuteTask(pid); }, intervalMs, initialDelayMs);
  }

  // Register a function to be executed every x ms
  TaskPID registerTimedTask(const std::string &name, Function func, int32_t intervalMs, int32_t initialDelayMs = 0) {
    TaskPID handle = _nextTaskPID++;
    TimedTaskInfo taskInfo{{handle, name, func, 0, 0, 0}, initialDelayMs == 0, intervalMs, _systickCounter + initialDelayMs};
    if(initialDelayMs == 0){
      taskInfo.nextExecution += intervalMs;
    }
    _timedTasks.push_back(taskInfo);
    return handle;
  }

  // Register a function to be executed once after y ms
  TaskPID registerDelayedTask(const std::string &name, Function func, int32_t delayMs) {
    return registerTimedTask(name, func, -1, delayMs);
  }

  bool modifyTimedTaskInterval(TaskPID handle, int32_t newIntervalMs) {
    for (auto &task : _timedTasks) {
      if (task.info.pid == handle) {
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

  TaskPID registerSignalTask(ITask *task, SignalFilter filter) {
    return registerSignalTask(task->getName(), [task](TaskPID pid) { return task->ExecuteTask(pid); }, filter);
  }

  TaskPID registerSignalTask(const std::string &name, Function func, SignalFilter filter) {
    TaskPID handle = _nextTaskPID++;
    _signalTasks.push_back({{handle, name, func, 0, 0, 0}, filter, false});
    return handle;
  }

  TaskPID registerSignalTask(ITask *task, Signal signal) {
    return registerSignalTask(task, {signal, 0xFFFFFFFF});
  }

  TaskPID registerSignalTask(const std::string &name, Function func, Signal signal) {
    return registerSignalTask(name, func, {signal, 0xFFFFFFFF});
  }

  SignalFilter getSignalFilter(TaskPID handle) const {
    for (auto &task : _signalTasks) {
      if (task.info.pid == handle) {
        return task.filter;
      }
    }
    return {0, 0};
  }

  void triggerSignal(Signal signal) {
    for (auto &task : _signalTasks) {
      if(((signal ^ task.filter.signal) & task.filter.mask) == 0){
        task.execute = true;
      }
    }
  }

  void killTask(TaskPID handle) {
    _tasksToKill.push_back(handle);
  }

  // Start the mainloop
  void start();

  // Stop the mainloop
  void stop() { _running = false; }

  // Simulated SysTick functions
  uint32_t getSysTick() const { return _systickCounter; }

  void OuptutTaskInformation() const;

private:
  std::vector<RegularTaskInfo> _regularTasks;
  std::vector<TimedTaskInfo> _timedTasks;
  std::vector<SignalTaskInfo> _signalTasks;

  std::vector<TaskPID> _tasksToKill;

  uint32_t _loop_statistic[8];
  uint32_t _max_loop_time;
  int _loop_statistic_ptr;

  bool _running;
  uint32_t _systickCounter = 0;
  TaskPID _nextTaskPID = 0;

  Mainloop();

  void onMillisecond();
  void calculateStatistics(struct TaskInfo &task);
  void OuptutTaskInformation(const struct TaskInfo &task) const;

  static bool alarm_callback(repeating_timer *rt);
};