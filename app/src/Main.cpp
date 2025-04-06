#include "Console.h"
#include "pico/stdlib.h"
#include "tusb.h"
#include <iostream>

#include "Commands/version.h"

static void cdc_task(void);

int main() {
  stdio_init_all();

  tusb_init();

  Console console;

  console.registerCommand(std::make_shared<VersionCommand>());

  while (true) {
    tud_task();            // Handle USB tasks
    console.consoleTask(); // Handle console tasks
  }
  return 0;
}

// echo to either Serial0 or Serial1
// with Serial0 as all lower case, Serial1 as all upper case
static void echo_serial_port(uint8_t itf, uint8_t buf[], uint32_t count) {
  for (uint32_t i = 0; i < count; i++) {
    if (itf == 0) {
      // echo back 1st port as lower case
      if (isupper(buf[i]))
        buf[i] += 'a' - 'A';
    } else {
      // echo back additional ports as upper case
      if (islower(buf[i]))
        buf[i] -= 'a' - 'A';
    }

    tud_cdc_n_write_char(itf, buf[i]);

    if (buf[i] == '\r') {
      tud_cdc_n_write_char(itf, '\n');

      std::cout << "Goodbye, World!" << std::endl;
    }
  }
  tud_cdc_n_write_flush(itf);
}

//--------------------------------------------------------------------+
// USB CDC
//--------------------------------------------------------------------+
static void cdc_task(void) {
  uint8_t itf;

  for (itf = 0; itf < CFG_TUD_CDC; itf++) {
    if (tud_cdc_n_connected(itf)) {
      if (tud_cdc_n_available(itf)) {
        uint8_t buf[64];

        uint32_t count = tud_cdc_n_read(itf, buf, sizeof(buf));

        // echo back to both serial ports
        echo_serial_port(0, buf, count);
        echo_serial_port(1, buf, count);
      }
    }
  }
}
