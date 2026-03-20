// Stub: hardware/irq.h — IRQ types for host-side testing
#pragma once

#include <cstdint>

typedef void (*irq_handler_t)(void);

static inline void irq_set_exclusive_handler(uint, irq_handler_t) {}
static inline void irq_set_enabled(uint, bool) {}
static inline void irq_set_priority(uint, uint8_t) {}

#define UART0_IRQ 20
#define UART1_IRQ 21
#define DMA_IRQ_0 11
#define DMA_IRQ_1 12
