#pragma once

#include "../ICommand.h"
#include "LED/WS2812.h"
#include "Utils/base64.h"
#include <iostream>
#include <string>

class LedCommand : public ICommand {
public:
  // Constructor
  LedCommand(PIO pio, int number) : _pio(pio), _number(number) {}

  // Returns the name of the command
  const std::string getName() const override {
    return "led" + std::to_string(_number);
  }

  const std::string getHelp() const override {
    return "Usage: " + getName() + " open <pin> <led_count> [bits_per_led]\n"
           "       " + getName() + " status\n"
           "       " + getName() + " set <base64_pattern>\n"
           "       " + getName() + " add <base64_pattern> <- returns the number of the pattern buffer\n"
           "       " + getName() + " use <pattern_number>\n"
           "       " + getName() + " del <pattern_number>";
  }

  // Executes the command
  int execute(const std::vector<std::string> &args) override {
    if (args.size() < 2) {
      std::cout << getHelp() << std::endl;
      return -1; // Return -1 to indicate failure
    }

    if (args[1] == "open") {
      return open(args);
    }
    if (args[1] == "status") {
      return status(args);
    }
    if (args[1] == "set") {
      return set(args);
    }
    if (args[1] == "add") {
      return add(args);
    }
    if (args[1] == "use") {
      return use(args);
    }
    if (args[1] == "del") {
      return del(args);
    }

    return -1;
  }

private:
  // Add any private members or methods if needed
  PIO _pio;
  int _number;
  int _pin;
  int _led_num;
  int _bits_per_pixel;
  std::shared_ptr<WS2812> _led;

  int open(const std::vector<std::string> &args) {
    if (args.size() < 4) {
      std::cout << "Usage: " << args[0]
                << " open <pin> <led_count> [bits_per_led]" << std::endl;
      return -1; // Return -1 to indicate failure
    }
    if (_led) {
      std::cout << "LED already initialized, use \"" << args[0] << " status\""
                << std::endl;
      return -1; // Return -1 to indicate failure
    }
    _pin = std::stoi(args[2]);
    _led_num = std::stoi(args[3]);
    _bits_per_pixel = 24; // Default value
    if (args.size() > 4) {
      _bits_per_pixel = std::stoi(args[4]);
    }
    _led = std::make_shared<WS2812>(_pio, _pin, _led_num, _bits_per_pixel);
    return 0; // Return 0 to indicate success
  }

  int status(const std::vector<std::string> &args) {
    if (_led) {
      std::cout << "LED open on pin " << _pin << " for " << _led_num
                << " LEDs (" << _bits_per_pixel << " bits per LED)"
                << std::endl;
      return 0; // Return 0 to indicate success
    }

    std::cout << "LED not initialized" << std::endl;
    return -1; // Return -1 to indicate failure
  }

  std::vector<uint32_t> convert(const std::vector<uint8_t> &pattern_raw) {
    std::vector<uint32_t> pattern;
    for (size_t i = 0; i < (pattern_raw.size() - 3); i += 4) {
      uint32_t value = (static_cast<uint32_t>(pattern_raw[i + 0]) << 24) |
                       (static_cast<uint32_t>(pattern_raw[i + 1]) << 16) |
                       (static_cast<uint32_t>(pattern_raw[i + 2]) << 8) |
                       (static_cast<uint32_t>(pattern_raw[i + 3]) << 0);
      pattern.push_back(value);
    }
    return pattern;
  }

  int set(const std::vector<std::string> &args) {
    if (args.size() < 3) {
      std::cout << "Usage: " << args[0] << " set <base64_pattern>" << std::endl;
      return -1; // Return -1 to indicate failure
    }
    if (!_led) {
      std::cout << "LED not initialized" << std::endl;
      return -1; // Return -1 to indicate failure
    }
    std::string base64_pattern = args[2];
    std::vector<uint8_t> pattern_raw = Base64::decode(base64_pattern);
    std::vector<uint32_t> pattern = convert(pattern_raw);
    if (!_led->setPattern(pattern)) {
      std::cout << "Pattern size is less than the number of LEDs (got "
                << pattern.size() << ", bytes " << pattern_raw.size() << ")"
                << std::endl;
      return -1; // Return -1 to indicate failure
    }
    return 0; // Return 0 to indicate success
  }

  int add(const std::vector<std::string> &args) {
    if (args.size() < 3) {
      std::cout << "Usage: " << args[0]
                << " add <base64_pattern> <- returns the number of the "
                   "patternbuffer"
                << std::endl;
      return -1; // Return -1 to indicate failure
    }
    if (!_led) {
      std::cout << "LED not initialized" << std::endl;
      return -1; // Return -1 to indicate failure
    }
    std::string base64_pattern = args[2];
    std::vector<uint8_t> pattern_raw = Base64::decode(base64_pattern);
    std::vector<uint32_t> pattern = convert(pattern_raw);
    int pattern_number = _led->addPattern(pattern);
    if (pattern_number < 0) {
      std::cout << "Pattern size is less than the number of LEDs (got "
                << pattern.size() << ", bytes " << pattern_raw.size() << ")"
                << std::endl;
      return -1; // Return -1 to indicate failure
    }
    std::cout << "Pattern number: " << pattern_number << std::endl;
    return 0; // Return 0 to indicate success
  }

  int use(const std::vector<std::string> &args) {
    if (args.size() < 3) {
      std::cout << "Usage: " << args[0] << " use <pattern_number>" << std::endl;
      return -1; // Return -1 to indicate failure
    }
    if (!_led) {
      std::cout << "LED not initialized" << std::endl;
      return -1; // Return -1 to indicate failure
    }
    int pattern_number = std::stoi(args[2]);
    _led->setPattern(pattern_number);
    return 0; // Return 0 to indicate success
  }

  int del(const std::vector<std::string> &args) {
    if (args.size() < 3) {
      std::cout << "Usage: " << args[0] << " del <pattern_number>" << std::endl;
      return -1; // Return -1 to indicate failure
    }
    if (!_led) {
      std::cout << "LED not initialized" << std::endl;
      return -1; // Return -1 to indicate failure
    }
    int pattern_number = std::stoi(args[2]);
    _led->removePattern(pattern_number);
    return 0; // Return 0 to indicate success
  }
};