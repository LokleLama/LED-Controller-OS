// Stub: hardware/uart.h — UART types for host-side testing
#pragma once

#include <cstdint>
#include <cstddef>

typedef struct uart_hw {} *uart_inst_t;

extern struct uart_hw uart0_hw_instance;
extern struct uart_hw uart1_hw_instance;
#define uart0 (&uart0_hw_instance)
#define uart1 (&uart1_hw_instance)

static inline uint uart_init(uart_inst_t, uint) { return 0; }
static inline void uart_set_format(uart_inst_t, uint, uint, uint) {}
static inline void uart_set_hw_flow(uart_inst_t, bool, bool) {}
static inline void uart_set_fifo_enabled(uart_inst_t, bool) {}
static inline void uart_set_irq_enables(uart_inst_t, bool, bool) {}
static inline bool uart_is_readable(uart_inst_t) { return false; }
static inline uint8_t uart_getc(uart_inst_t) { return 0; }
static inline void uart_putc(uart_inst_t, char) {}
static inline void uart_puts(uart_inst_t, const char *) {}
static inline void uart_write_blocking(uart_inst_t, const uint8_t *, size_t) {}

#define UART_PARITY_NONE 0
#define UART_PARITY_EVEN 1
#define UART_PARITY_ODD  2
