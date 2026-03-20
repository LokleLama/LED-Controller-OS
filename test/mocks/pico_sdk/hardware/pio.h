// Stub: hardware/pio.h — PIO types for host-side testing
#pragma once

#include <cstdint>
#include <cstddef>

#ifndef uint
typedef unsigned int uint;
#endif

typedef struct pio_hw {
    volatile uint32_t txf[4];
} *PIO;

typedef struct {
    uint32_t clkdiv;
    uint32_t execctrl;
    uint32_t shiftctrl;
    uint32_t pinctrl;
} pio_sm_config;

struct pio_program {
    const uint16_t *instructions;
    uint8_t length;
    int8_t origin;
    int8_t pio_version;
};
typedef struct pio_program pio_program_t;

extern struct pio_hw pio0_hw_instance;
extern struct pio_hw pio1_hw_instance;
#define pio0 (&pio0_hw_instance)
#define pio1 (&pio1_hw_instance)

static inline int pio_claim_unused_sm(PIO, bool) { return 0; }
static inline uint pio_add_program(PIO, const pio_program_t *) { return 0; }
static inline void pio_sm_init(PIO, uint, uint, const pio_sm_config *) {}
static inline void pio_sm_set_enabled(PIO, uint, bool) {}
static inline pio_sm_config pio_get_default_sm_config(void) { return {}; }
static inline void sm_config_set_sideset_pins(pio_sm_config *, uint) {}
static inline void sm_config_set_out_shift(pio_sm_config *, bool, bool, uint) {}
static inline void sm_config_set_fifo_join(pio_sm_config *, int) {}
static inline void sm_config_set_clkdiv(pio_sm_config *, float) {}
static inline void sm_config_set_out_pins(pio_sm_config *, uint, uint) {}
static inline void sm_config_set_set_pins(pio_sm_config *, uint, uint) {}
static inline void sm_config_set_sideset(pio_sm_config *, uint, bool, bool) {}
static inline void sm_config_set_wrap(pio_sm_config *, uint, uint) {}
static inline void pio_gpio_init(PIO, uint) {}
static inline void pio_sm_set_consecutive_pindirs(PIO, uint, uint, uint, bool) {}
static inline uint32_t pio_sm_get_pc(PIO, uint) { return 0; }

void mock_pio_put(PIO pio, uint sm, uint32_t data);
static inline void pio_sm_put_blocking(PIO pio, uint sm, uint32_t data) {
    mock_pio_put(pio, sm, data);
}

enum pio_fifo_join {
    PIO_FIFO_JOIN_NONE = 0,
    PIO_FIFO_JOIN_TX = 1,
    PIO_FIFO_JOIN_RX = 2,
};

static inline int pio_get_index(PIO) { return 0; }
static inline uint pio_get_dreq(PIO, uint, bool) { return 0; }
