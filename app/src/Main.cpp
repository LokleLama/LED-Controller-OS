#include "pico/stdlib.h"
#include "tusb.h"
#include <iostream>

#include "hardware/clocks.h"

static void cdc_task(void);

void countdown(uint n) {
  for (uint i = 0; i < n; i++) {
    std::cout << n - i << std::endl;
    sleep_ms(1000);
  }
}

int main() {
  stdio_init_all();

  tusb_init();

  std::cout << "Clock Rate: " << clock_get_hz(clk_sys) << std::endl;

  while (true) {
    tud_task(); // Handle USB tasks
    cdc_task(); // Handle CDC tasks

    // Example: Send data to the first CDC interface (console)
    // tud_cdc_n_write_str(0, "Console: Hello, USB!\r\n");
    // tud_cdc_n_write_flush(0);

    // Example: Send data to the second CDC interface (UART access)
    // tud_cdc_n_write_str(1, "UART: Hello, USB!\r\n");
    // tud_cdc_n_write_flush(1);

    // sleep_ms(1000);
  }
  std::cout << "Goodbye, World!" << std::endl;
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

    if (buf[i] == '\r')
      tud_cdc_n_write_char(itf, '\n');
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
