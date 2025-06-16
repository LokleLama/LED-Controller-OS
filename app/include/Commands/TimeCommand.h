#pragma once

#include "ICommand.h"
#include "RTC/PicoTime.h"
#include <ctime>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

class TimeCommand : public ICommand {
public:
  // Returns the name of the command
  const std::string getName() const override { return "time"; }

  // Constructor that takes a shared pointer to PicoTime
  TimeCommand(std::shared_ptr<PicoTime> picoTime)
      : picoTime(std::move(picoTime)) {}

  // Executes the command
  int execute(const std::vector<std::string> &args) override {
    auto timeInfo = picoTime->getTimeInfo();
    if (args.size() <= 1 || args.size() > 1 && args[1] == "get") {
      // Get the current time
      char buffer[20];
      std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeInfo);
      std::cout << "Current time: " << buffer << std::endl;
      return 0;
    } else if (args.size() > 2 && args[1] == "set") {
      if (!convertStringToStructTm(args, timeInfo)) {
        std::cout << "Invalid time format. Use YYYY-MM-DD HH:MM:SS"
                  << std::endl;
        return -1;
      }
      char buffer[20];
      std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeInfo);
      std::cout << "Trying to set Time to: " << buffer << std::endl;
      picoTime->setTime(timeInfo);
      return execute({});
    }

    std::cout << "Usage: time get | time set YYYY-MM-DD HH:MM:SS" << std::endl;
    return -1; // Command not recognized
  }

private:
  std::shared_ptr<PicoTime> picoTime;

  bool convertStringToDate(const std::string &str, std::tm &newTime) {
    // this function converts a date string in the format "YYYY-MM-DD" or
    // "DD.MM.YYYY" to a struct tm
    // Check if the string is in the format "YYYY-MM-DD"
    if (str.find('-') != std::string::npos) {
      // Split the string by '-'
      std::istringstream dateStream(str);
      int year, month, day;
      char dash1, dash2;
      if (!(dateStream >> year >> dash1 >> month >> dash2 >> day) ||
          dash1 != '-' || dash2 != '-') {
        return false; // Invalid date format
      }
      // Adjust month for struct tm (0-11)
      newTime.tm_year = year - 1900; // tm_year is years since 1900
      newTime.tm_mon = month - 1;    // tm_mon is 0-11
      newTime.tm_mday = day;         // tm_mday is 1-31
      return true;
    }
    // Check if the string is in the format "DD.MM.YYYY"
    if (str.find('.') != std::string::npos) {
      // Split the string by '.'
      std::istringstream dateStream(str);
      int day, month, year;
      char dot1, dot2;
      if (!(dateStream >> day >> dot1 >> month >> dot2 >> year) ||
          dot1 != '.' || dot2 != '.') {
        return false; // Invalid date format
      }
      if (year < 1900) {
        year += 2000; // Adjust year if it's less than 1900
        std::cout << "Year adjusted to: " << year << std::endl;
      }
      newTime.tm_year = year - 1900; // tm_year is years since 1900
      newTime.tm_mon = month - 1;    // tm_mon is 0-11
      newTime.tm_mday = day;         // tm_mday is 1-31
      return true;
    }
    return false;
  }

  bool convertStringToTime(const std::string &str, std::tm &newTime) {
    // this function converts a time string in the format "HH:MM:SS" to a struct
    // tm
    std::string timePart = str;
    std::replace(timePart.begin(), timePart.end(), ':', ' ');
    std::istringstream timeStream(timePart);
    int hour, minute, second = 0;
    if (!(timeStream >> hour >> minute)) {
      return false; // Invalid time format
    }
    if (timeStream >> second) {
      // If seconds are provided, read them
    }
    newTime.tm_hour = hour;  // tm_hour is 0-23
    newTime.tm_min = minute; // tm_min is 0-59
    newTime.tm_sec = second; // tm_sec is 0-60 (leap seconds)
    newTime.tm_isdst = -1;   // Not considering daylight saving time

    return true;
  }

  bool convertStringToStructTm(const std::vector<std::string> &args,
                               std::tm &newTime) {
    // this function splits a string with the format "YYYY-MM-DD HH:MM:SS" or
    // "DD.MM.YYYY HH:MM:SS" into date and time parts and converts them to a
    // struct tm it will also handle cases where only date or time is provided
    std::string datePart, timePart;
    datePart = args[2];
    if (args.size() >= 4) {
      timePart = args[3];
    } else {
      timePart = "";
    }

    if (datePart.find(':') != std::string::npos) {
      timePart = datePart;
      datePart = "";
    }

    if (!datePart.empty() && !convertStringToDate(datePart, newTime)) {
      return false; // Invalid date format
    }
    if (!timePart.empty() && !convertStringToTime(timePart, newTime)) {
      return false; // Invalid time format
    }
    std::mktime(&newTime);
    return true;
  }
};