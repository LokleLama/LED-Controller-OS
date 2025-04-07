#include "Console.h"
#include "pico/stdlib.h"
#include "tusb.h"
#include <iostream>

#include "Commands/echo.h"
#include "Commands/version.h"

static void cdc_task(void);

int main() {
  stdio_init_all();

  tusb_init();

  Console console;

  console.registerCommand(std::make_shared<VersionCommand>());
  console.registerCommand(std::make_shared<EchoCommand>());

  while (true) {
    tud_task();            // Handle USB tasks
    console.consoleTask(); // Handle console tasks
  }
  return 0;
}
