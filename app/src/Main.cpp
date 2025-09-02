#include "Config.h"
#include "Console.h"
#include "hardware/irq.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"
#include "tusb.h"
#include <iostream>
#include <memory>
#include <sys/types.h>
#include <vector>

#include "VariableStore/VariableStore.h"

#include "Commands/EchoCommand.h"
#include "Commands/EnvCommand.h"
#include "Commands/EvalCommand.h"
#include "Commands/GetCommand.h"
#include "Commands/HelpCommand.h"
#include "Commands/LedCommand.h"
#include "Commands/SetCommand.h"
#include "Commands/TimeCommand.h"
#include "Commands/VersionCommand.h"

#include "Commands/ReadCommand.h"

#include "HLKLogger.h"
#include "HLKStack/HLKPackageFinder.h"
#include "HexLogger.h"
#include "IRQFifo.h"

#include "RTC/PicoTime.h"

#include "LED/WS2812.h"
#include "Mainloop.h"

static void uart_task(void);
static std::shared_ptr<IComLogger> logger;
static std::shared_ptr<IVariable> output_distance;
static std::shared_ptr<IVariable> distance;
static std::shared_ptr<PicoTime> picoTime;
static HLKPackageFinder package_finder;
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

  Mainloop mainloop;
  VariableStore variableStore;
  Console console(variableStore);

  picoTime = std::make_shared<PicoTime>();

  console.registerCommand(std::make_shared<VersionCommand>());
  console.registerCommand(std::make_shared<EchoCommand>());
  console.registerCommand(std::make_shared<EvalCommand>());
  console.registerCommand(std::make_shared<TimeCommand>(picoTime));
  console.registerCommand(std::make_shared<SetCommand>(variableStore));
  console.registerCommand(std::make_shared<GetCommand>(variableStore));
  console.registerCommand(std::make_shared<EnvCommand>(variableStore));
  console.registerCommand(std::make_shared<HelpCommand>(console));
  console.registerCommand(std::make_shared<LedCommand>(pio0, 1));
  console.registerCommand(std::make_shared<LedCommand>(pio0, 2));
  console.registerCommand(std::make_shared<LedCommand>(pio0, 3));
  console.registerCommand(std::make_shared<LedCommand>(pio0, 4));

  console.registerCommand(std::make_shared<ReadCommand>());

  variableStore.addVariable("uart0.baud", 115200);
  variableStore.addVariable("uart0.format", "8n1");
  variableStore.addVariable("uart0.log", "none");
  output_distance = variableStore.addBoolVariable("distance", false);
  distance = variableStore.addVariable("dist", 0.0f);

  variableStore.registerCallback(
      "uart0.baud", [](const std::string &key, const std::string &value) {
        int baudRate = std::stoi(value, nullptr, 10);
        int actual = uart_set_baudrate(uart0, baudRate);
        std::cout << "Baud rate changed to " << actual
                  << " (requested: " << baudRate << ")" << std::endl;
        return true;
      });
  variableStore.registerCallback(
      "uart0.format", [](const std::string &key, const std::string &value) {
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
  variableStore.registerCallback("uart0.log", [](const std::string &key,
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

  mainloop.registerRegularTask([&]() {
    tud_task(); // TinyUSB task
    return true;
  });
  mainloop.registerRegularTask(&console); // Console task
  mainloop.registerRegularTask([&]() {
    uart_task(); // UART task
    return true;
  });

  mainloop.start();

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
      auto pack = package_finder.fastDistanceFinder(buf, count);
      if (pack) {
        distance->set(pack->getDistance());
        if (output_distance->asBool()) {
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
