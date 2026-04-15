#include "Mainloop.h"

#include "hardware/clocks.h"
#include "hardware/irq.h"

#include "Utils/ValueConverter.h"
#include "Utils/Signal.h"

#include <time.h>
#include <cstring>
#include <iomanip>

Mainloop& Mainloop::getInstance() {
    static Mainloop instance;
    return instance;
}

Mainloop::Mainloop() : _running(false), _systickCounter(0), _loop_statistic_ptr(0), _max_loop_time(0) {
  memset(_loop_statistic, 0, sizeof(_loop_statistic));
}

// Static callback function for the alarm
bool Mainloop::alarm_callback(repeating_timer *rt) {
  Mainloop *instance = static_cast<Mainloop *>(rt->user_data);
  instance->onMillisecond();
  return true; // Return true to keep the timer running
}

// Start the mainloop
void Mainloop::start() {
  _running = true;

  // Create a repeating timer
  repeating_timer_t timer;
  add_repeating_timer_ms(1, alarm_callback, this, &timer);

  while (_running) {
    // Execute timed tasks
    uint64_t loop_start = time_us_64();
    for (auto &task : _timedTasks) {
      if (task.execute) {
        task.info.startTime = time_us_64();
        if(!task.info.func(task.info.pid)){
          task.intervalMs = -1; // mark the Task for removal
        }
        calculateStatistics(task.info);
        task.execute = false;
      }
    }

    for (auto &task : _signalTasks) {
      if (task.execute) {
        task.info.startTime = time_us_64();
        auto rerun = task.info.func(task.info.pid);
        calculateStatistics(task.info);
        task.execute = rerun;
      }
    }

    uint32_t currentTime = _systickCounter;
    // Execute regular tasks
    for (auto &task : _regularTasks) {
      if(task.sleepUntil > currentTime){
        continue;
      }
      task.info.startTime = time_us_64();
      task.info.func(task.info.pid);
      calculateStatistics(task.info);
    }

    // Remove completed timed tasks
    for (int n = 0; n < _timedTasks.size(); n++) {
      if (_timedTasks[n].intervalMs < 0) {
        _timedTasks.erase(_timedTasks.begin() + n);
        n--;
      }
    }

    for (int n = 0; n < _tasksToKill.size(); n++) {
      TaskPID handle = _tasksToKill[n];
      bool found = false;
      for (int i = 0; i < _regularTasks.size(); i++) {
        if (_regularTasks[i].info.pid == handle) {
          _regularTasks.erase(_regularTasks.begin() + i);
          found = true;
          break;
        }
      }
      if (!found) {
        for (int i = 0; i < _timedTasks.size(); i++) {
          if (_timedTasks[i].info.pid == handle) {
            _timedTasks.erase(_timedTasks.begin() + i);
            found = true;
            break;
          }
        }
      }
      if (!found) {
        for (int i = 0; i < _signalTasks.size(); i++) {
          if (_signalTasks[i].info.pid == handle) {
            _signalTasks.erase(_signalTasks.begin() + i);
            break;
          }
        }
      }
    }
    _tasksToKill.clear();

    uint64_t total_loop_time = time_us_64() - loop_start;
    if(total_loop_time <= static_cast<uint64_t>(static_cast<uint32_t>(-1))){
      _loop_statistic[_loop_statistic_ptr] = total_loop_time;
      if(total_loop_time > _max_loop_time){
        _max_loop_time = total_loop_time;
      }
    }else{
      _loop_statistic[_loop_statistic_ptr] = static_cast<uint32_t>(-1);
    }
    _loop_statistic_ptr++;
    if(_loop_statistic_ptr >= 8){
      _loop_statistic_ptr = 0;
    }
  }

  // Cancel the timer when the loop stops
  cancel_repeating_timer(&timer);
}

void Mainloop::calculateStatistics(struct TaskInfo &task){
  uint64_t stopTime = time_us_64();
  if(task.startTime > stopTime){
    return;
  }
  uint32_t executionTime = stopTime - task.startTime;
  task.meanTime = (task.meanTime + executionTime) / 2;
  if(executionTime > task.maxTime){
    task.maxTime = executionTime;
  }
}

// Function to be called every millisecond
void Mainloop::onMillisecond() {
  _systickCounter++;
  for (auto &task : _timedTasks) {
    if (_systickCounter == task.nextExecution) {
      task.execute = true;
      task.nextExecution += task.intervalMs;
    }
  }
}

void Mainloop::OuptutTaskInformation() {
  float total_loop_time = 0;
  for(int i = 0; i < 8; i++){
    total_loop_time += _loop_statistic[i];
  }
  total_loop_time /= 80;
  std::cout << "Approximate CPU Load (per 1 ms): " << std::fixed << std::setprecision(2) << total_loop_time << "% (max time: " << _max_loop_time << "us)" << std::endl;

  std::cout << "Regular Tasks:" << std::endl;
  std::cout << " PID - Name (Mean Time / Max Time)" << std::endl;
  for (const auto &task : _regularTasks) {
    OuptutTaskInformation(task.info);
    if (task.sleepUntil > _systickCounter){
      std::cout << " [Sleeping for " << (task.sleepUntil - _systickCounter) << " ms]";
    } 
    std::cout << std::endl;
  }

  std::cout << std::endl << "Timed Tasks:" << std::endl;
  std::cout << " PID - Name (Mean Time / Max Time)" << std::endl;
  for (const auto &task : _timedTasks) {
    OuptutTaskInformation(task.info);
    std::cout << " [Interval: " << task.intervalMs << " ms, next execution in " << (task.nextExecution - _systickCounter) << " ms]" << std::endl;
  }

  std::cout << std::endl << "Signal waiting Tasks:" << std::endl;
  std::cout << " PID - Name (Mean Time / Max Time)" << std::endl;
  for (const auto &task : _signalTasks) {
    OuptutTaskInformation(task.info);
    std::cout << " [Signal: " << SignalConverter::toString(task.signal) << "]" << std::endl;
  }
}

void Mainloop::OuptutTaskInformation(const struct TaskInfo &task) {
  std::cout << " " << task.pid <<" - " << task.name << " (" << task.meanTime << " us / " << task.maxTime << " us)";
}
