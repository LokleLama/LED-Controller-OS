#include "Mainloop.h"

#include "hardware/clocks.h"
#include "hardware/irq.h"

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
        if(!task.func()){
          task.intervalMs = -1; // mark the Task for removal
        }
        task.execute = false;
        task.nextExecution = _systickCounter + task.intervalMs;
      }
    }

    // Execute regular tasks
    for (auto &task : _regularTasks) {
      task();
    }

    // Remove completed timed tasks
    for (int n = 0; n < _timedTasks.size(); n++) {
      if (_timedTasks[n].intervalMs == 0) {
        _timedTasks.erase(_timedTasks.begin() + n);
        n--;
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
}
