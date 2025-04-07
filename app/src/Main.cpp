#include "Console.h"
#include "hardware/irq.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"
#include "tusb.h"
#include <iostream>

#include "Commands/echo.h"
#include "Commands/set.h"
#include "Commands/version.h"

static const int UART_INTERFACE_NUMBER = 1;

static void uart_task(void);

int main() {
  stdio_init_all();

  tusb_init();

  uart_init(uart0, 115000);
  uart_set_hw_flow(uart0, false, false);
  gpio_set_function(0, GPIO_FUNC_UART);
  gpio_set_function(1, GPIO_FUNC_UART);

  Console console;

  console.registerCommand(std::make_shared<VersionCommand>());
  console.registerCommand(std::make_shared<EchoCommand>());
  console.registerCommand(std::make_shared<SetCommand>());

  while (true) {
    tud_task();            // Handle USB tasks
    console.consoleTask(); // Handle console tasks
    uart_task();
  }
  return 0;
}

static void uart_task(void) {
  if (tud_cdc_n_available(UART_INTERFACE_NUMBER)) {
    uint8_t buf[64];

    uint32_t count = tud_cdc_n_read(UART_INTERFACE_NUMBER, buf, sizeof(buf));

    uart_write_blocking(uart0, buf, count);
  }
}
