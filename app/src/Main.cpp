#include "Config.h"
#include "Console.h"
#include "hardware/irq.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"
#include "tusb.h"
#include <iostream>
#include <memory>

#include "VariableStore.h"

#include "Commands/echo.h"
#include "Commands/env.h"
#include "Commands/get.h"
#include "Commands/set.h"
#include "Commands/version.h"

#include "HLKLogger.h"
#include "HLKStack/HLKPackageFinder.h"
#include "HexLogger.h"
#include "IRQFifo.h"

static void uart_task(void);
static std::shared_ptr<IComLogger> logger;
static std::shared_ptr<HLKPackageFinder> output_dispance;
static IRQFifo uart_fifo(128);

// UART interrupt handler
void on_uart_rx() {
  while (uart_is_readable(uart0)) {
    uart_fifo.push(uart_getc(uart0));
  }
}

int main() {
  stdio_init_all();

  tusb_init();

  gpio_init(0);
  gpio_set_dir(0, GPIO_OUT);
  gpio_put(0, 1);

  gpio_init(1);
  gpio_set_dir(1, GPIO_IN);
  gpio_pull_up(1);

  uart_init(uart0, 115000);
  uart_set_hw_flow(uart0, false, false);
  gpio_set_function(0, GPIO_FUNC_UART);
  gpio_set_function(1, GPIO_FUNC_UART);
  // Enable UART interrupts
  irq_set_exclusive_handler(UART0_IRQ, on_uart_rx);
  irq_set_enabled(UART0_IRQ, true);
  uart_set_irq_enables(uart0, true, false);

  VariableStore variableStore;
  Console console(variableStore);

  console.registerCommand(std::make_shared<VersionCommand>());
  console.registerCommand(std::make_shared<EchoCommand>());
  console.registerCommand(std::make_shared<SetCommand>(variableStore));
  console.registerCommand(std::make_shared<GetCommand>(variableStore));
  console.registerCommand(std::make_shared<EnvCommand>(variableStore));

  variableStore.setVariable("baud", "115200");
  variableStore.setVariable("format", "8n1");
  variableStore.setVariable("log", "none");
  variableStore.setVariable("distance", "false");
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
  variableStore.registerCallback("log", [](const std::string &key,
                                           const std::string &value) {
    if (value == "none") {
      logger.reset();
    } else if (value == "hex") {
      logger = std::make_shared<HexLogger>();
    } else if (value == "hlk") {
      logger = std::make_shared<HLKLogger>();
    } else {
      std::cout << "Invalid log value, use 'none', 'hex' or 'hlk'" << std::endl;
      return false;
    }
    std::cout << "Log changed to " << value << std::endl;
    return true;
  });
  variableStore.registerCallback(
      "distance", [](const std::string &key, const std::string &value) {
        if (value == "true") {
          output_dispance = std::make_shared<HLKPackageFinder>();
          return true;
        } else if (value == "false") {
          output_dispance.reset();
          return true;
        }
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

    if (logger) {
      logger->Transmitting(buf, count);
    }

    uart_write_blocking(uart0, buf, count);
  }
  if (!uart_fifo.isEmpty()) {
    uint8_t buf[64];
    int count = uart_fifo.readAvailable(buf, sizeof(buf));
    if (count > 0) {
      if (output_dispance) {
        auto pack = output_dispance->findPackage(buf, count);
        if (pack && pack->getType() == IHLKPackage::Type::Minimal) {
          std::cout << pack->toString() << std::endl;
        }
      }
      if (logger) {
        logger->Receiving(buf, count);
      }
      tud_cdc_n_write(UART_INTERFACE_NUMBER, buf, count);
    }
    tud_cdc_n_write_flush(UART_INTERFACE_NUMBER);
  }
}
