#include "Mainloop.h"

#include "hardware/clocks.h"
#include "hardware/irq.h"

#include "Utils/ValueConverter.h"

#include <time.h>

Mainloop& Mainloop::getInstance() {
    static Mainloop instance;
    return instance;
}

Mainloop::Mainloop() : _running(false), _systickCounter(0) {}

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
    for (auto &task : _timedTasks) {
      if (task.execute) {
        task.info.startTime = time_us_64();
        if(!task.info.func()){
          task.intervalMs = -1; // mark the Task for removal
        }
        calculateStatistics(task.info);
        task.execute = false;
      }
    }

    for (auto &task : _signalTasks) {
      if (task.execute) {
        task.info.startTime = time_us_64();
        auto rerun = task.info.func();
        calculateStatistics(task.info);
        task.execute = rerun;
      }
    }

    // Execute regular tasks
    for (auto &task : _regularTasks) {
      task.startTime = time_us_64();
      task.func();
      calculateStatistics(task);
    }

    // Remove completed timed tasks
    for (int n = 0; n < _timedTasks.size(); n++) {
      if (_timedTasks[n].intervalMs < 0) {
        _timedTasks.erase(_timedTasks.begin() + n);
        n--;
      }
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
  std::cout << "Regular Tasks:" << std::endl;
  std::cout << " PID - Name (Mean Time / Max Time)" << std::endl;
  for (const auto &task : _regularTasks) {
    OuptutTaskInformation(task);
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
    std::cout << " [Signal: " << ValueConverter::toString(task.signal, IntegerStringFormat::HEX) << "]" << std::endl;
  }
}

void Mainloop::OuptutTaskInformation(const struct TaskInfo &task) {
  std::cout << " " << task.handle <<" - " << task.name << " (" << task.meanTime << " us / " << task.maxTime << " us)";
}
