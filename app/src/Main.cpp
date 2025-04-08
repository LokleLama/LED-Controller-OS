#include "Console.h"
#include "hardware/irq.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"
#include "tusb.h"
#include <iostream>

#include "VariableStore.h"

#include "Commands/echo.h"
#include "Commands/env.h"
#include "Commands/get.h"
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

  VariableStore variableStore;
  Console console(variableStore);

  console.registerCommand(std::make_shared<VersionCommand>());
  console.registerCommand(std::make_shared<EchoCommand>());
  console.registerCommand(std::make_shared<SetCommand>(variableStore));
  console.registerCommand(std::make_shared<GetCommand>(variableStore));
  console.registerCommand(std::make_shared<EnvCommand>(variableStore));

  variableStore.setVariable("baud", "115200");
  variableStore.setVariable("format", "8n1");
  variableStore.registerCallback(
      "baud", [](const std::string &key, const std::string &value) {
        int baudRate = std::stoi(value, nullptr, 10);
        int actual = uart_set_baudrate(uart0, baudRate);
        std::cout << "Baud rate changed to " << actual
                  << " (requested: " << baudRate << ")" << std::endl;
        return true;
      });
  variableStore.registerCallback(
      "format", [](const std::string &key, const std::string &value) {
        if (value.size() < 3) {
          std::cout << "Invalid format value" << std::endl;
          return false;
        }
        int bits = value[0] - '0';
        char parity = value[1];
        int stopBits = value[2] - '0';

        if (bits < 5 || bits > 8 ||
            (parity != 'n' && parity != 'o' && parity != 'e') ||
            (stopBits != 1 && stopBits != 2)) {
          std::cout << "Invalid format value" << std::endl;
          return false;
        }
        // Implement UART0 format change logic here
        // Example: UART0.setFormat(bits, parity, stopBits);
        std::cout << "Format changed to " << bits << parity << stopBits
                  << " (not implemented yet)" << std::endl;
        return false;
      });

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
