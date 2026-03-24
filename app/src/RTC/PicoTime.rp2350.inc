#include "RTC/PicoTime.h"

// RP2350 — no hardware RTC, use software time via pico_stdlib timer

#include "hardware/timer.h"

static time_t s_base_time = 0;
static uint64_t s_base_us = 0;

PicoTime::PicoTime() {
  // Set a default base time (2025-06-10 23:33:15)
  std::tm def = {};
  def.tm_year = 2025 - 1900;
  def.tm_mon = 5;   // June
  def.tm_mday = 10;
  def.tm_hour = 23;
  def.tm_min = 33;
  def.tm_sec = 15;
  s_base_time = mktime(&def);
  s_base_us = time_us_64();
}

void PicoTime::setTime(const std::tm &timeinfo) {
  std::tm copy = timeinfo;
  s_base_time = mktime(&copy);
  s_base_us = time_us_64();
}

void PicoTime::setTime(time_t tval) {
  s_base_time = tval;
  s_base_us = time_us_64();
}

std::tm PicoTime::getTimeInfo() {
  time_t now = GetTime();
  std::tm timeinfo = {};
  std::tm *p = localtime(&now);
  if (p) {
    timeinfo = *p;
  }
  return timeinfo;
}

time_t PicoTime::GetTime() {
  uint64_t elapsed_us = time_us_64() - s_base_us;
  return s_base_time + static_cast<time_t>(elapsed_us / 1000000ULL);
}
