#pragma once

#include "pico/bootrom.h"

#include "ICommand.h"
#include <iostream>

class BootCommand : public ICommand {
public:
  // Returns the name of the command
  const std::string getName() const override { return "boot"; }

  const std::string getHelp() const override {
    return "Usage: boot\n"
           "       Makes the Raspberry Pi Pico enter the bootloader mode, allowing you to flash new firmware by holding the BOOTSEL button while plugging in the USB cable.";
  }

  // Executes the command
  int execute(const std::vector<std::string> &args) override {
    // Reset into bootloader mode by setting boot mode and resetting
    reset_usb_boot(0, 0);
    
    return 0; // Return 0 to indicate success
  }
};