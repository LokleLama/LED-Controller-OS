#pragma once

#include "pico/stdlib.h"
#include <ctime>

class PicoTime {
public:
  PicoTime();

  // Set the RTC time using a std::tm struct
  void setTime(const std::tm &timeinfo);
  // Set the RTC time using a time_t value
  void setTime(time_t tval);

  std::tm getTimeInfo();
  // Get the RTC time as a time_t value
  time_t GetTime();
};