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

/*
  // Executes the command
  int execute(std::vector<std::string> args) override {
    if (args.size() < 3) {
      return showUsage();
    }

    if (args[1] == "baud") {
      int baudRate = std::stoi(args[2], nullptr, 10);
      setBaudRate(baudRate);
    } else if (args[1] == "format") {
      if (args[2].size() < 3) {
        return showUsage();
      }
      int bits = args[2][0] - '0';
      char parity = args[2][1];
      int stopBits = args[2][2] - '0';

      if (bits < 5 || bits > 8 ||
          (parity != 'n' && parity != 'o' && parity != 'e') ||
          (stopBits != 1 && stopBits != 2)) {
        return showUsage();
      }
      setFormat(bits, parity, stopBits);
    } else {
      return showUsage();
    }

    return 0;
  }


void setBaudRate(int baudRate) {
  int actual = uart_set_baudrate(uart0, baudRate);
  std::cout << "Setting baud rate to " << actual
            << " (requested: " << baudRate << ")" << std::endl;
}

void setFormat(int bits, char parity, int stopBits) {
  // Implement UART0 format change logic here
  // Example: UART0.setFormat(bits, parity, stopBits);
}

int showUsage() {
  std::cout << "Usage: set <baud|format> <value>" << std::endl;
  std::cout << "       Example: set baud 115200" << std::endl;
  std::cout << "       Example: set format 8n1" << std::endl;
  return -1; // Return -1 to indicate failure
}
  */