#include "RTC/PicoTime.h"
#include "hardware/rtc.h"

PicoTime::PicoTime() {
  rtc_init();
  // Initialize the RTC with a default time if needed
  datetime_t t;
  rtc_get_datetime(&t);
  if (t.year < 2020) { // Check if RTC is uninitialized or has an old date
    t.year = 2025;     // Set a default year
    t.month = 6;       // Set a default month
    t.day = 10;        // Set a default day
    t.dotw = 2;        // Set to Tuesday
    t.hour = 23;       // Set to 23 hours
    t.min = 33;        // Set to 33 minutes
    t.sec = 15;        // Set to 15 seconds
    rtc_set_datetime(&t);
  }
}

// Set the RTC time using a std::tm struct
void PicoTime::setTime(const std::tm &timeinfo) {
  datetime_t t = {
      .year = static_cast<int16_t>(timeinfo.tm_year + 1900),
      .month = static_cast<int8_t>(timeinfo.tm_mon + 1),
      .day = static_cast<int8_t>(timeinfo.tm_mday),
      .dotw = static_cast<int8_t>(
          (timeinfo.tm_wday == 0) ? 7 : timeinfo.tm_wday), // 1=Monday, 7=Sunday
      .hour = static_cast<int8_t>(timeinfo.tm_hour),
      .min = static_cast<int8_t>(timeinfo.tm_min),
      .sec = static_cast<int8_t>(timeinfo.tm_sec)};
  rtc_set_datetime(&t);
}

// Set the RTC time using a time_t value
void PicoTime::setTime(time_t tval) {
  std::tm *timeinfo = localtime(&tval);
  if (timeinfo) {
    setTime(*timeinfo);
  }
}

// Read the RTC time into a std::tm struct
std::tm PicoTime::getTimeInfo() {
  datetime_t t;
  rtc_get_datetime(&t);
  std::tm timeinfo = {};
  timeinfo.tm_year = t.year - 1900;
  timeinfo.tm_mon = t.month - 1;
  timeinfo.tm_mday = t.day;
  timeinfo.tm_wday = t.dotw - 1;
  timeinfo.tm_hour = t.hour;
  timeinfo.tm_min = t.min;
  timeinfo.tm_sec = t.sec;
  return timeinfo;
}

// Get the RTC time as a time_t value
time_t PicoTime::GetTime() {
  std::tm timeinfo = getTimeInfo();
  return mktime(&timeinfo);
}