#include "RTC/PicoTime.h"

#if defined(PICO_RP2350) && PICO_RP2350
  #include "PicoTime.rp2350.inc"
#else
  #include "PicoTime.rp2040.inc"
#endif