#include "Mainloop.h"

#include "hardware/clocks.h"
#include "hardware/irq.h"

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
    {
      // Execute timed tasks
      for (auto &task : _timedTasks) {
        if (task.execute) {
          task.func();
          task.execute = false;
          task.nextExecution = _systickCounter + task.intervalMs;
        }
      }
    }
    {
      // Execute regular tasks
      for (auto &task : _regularTasks) {
        task();
      }
    }
    {
      // Execute delayed tasks
      for (auto &task : _delayedTasks) {
        if (task.execute) {
          task.func();
          task.execute = false;
          task.executionCounter--;
        }
      }
      for (int n = 0; n < _delayedTasks.size(); n++) {
        if (_delayedTasks[n].executionCounter == 0) {
          _delayedTasks.erase(_delayedTasks.begin() + n);
          n--;
        }
      }
    }
  }

  // Cancel the timer when the loop stops
  cancel_repeating_timer(&timer);
}

// Function to be called every millisecond
void Mainloop::onMillisecond() {
  _systickCounter++;
  for (auto &task : _timedTasks) {
    if (_systickCounter == task.nextExecution) {
      task.execute = true;
    }
  }
  for (auto &task : _delayedTasks) {
    if (_systickCounter == task.nextExecution) {
      task.execute = true;
    }
  }
}
