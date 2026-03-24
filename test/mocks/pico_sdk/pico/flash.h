// Stub: pico/flash.h — flash_safe_execute for host-side testing
#pragma once

#include <cstdint>
#include <cstddef>

typedef void (*flash_safety_helper_t)(void *);

static inline int flash_safe_execute(flash_safety_helper_t func, void *param, uint32_t) {
    func(param);
    return 0;
}
