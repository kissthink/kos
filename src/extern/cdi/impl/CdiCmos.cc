#include "cdi/cmos.h"

// KOS
#include "mach/platform.h"
#include "util/basics.h"

uint8_t cdi_cmos_read(uint8_t index) {
  out8( 0x70, index );
  return in8( 0x71 );
}

void cdi_cmos_write(uint8_t index, uint8_t value) {
  out8( 0x70, index );
  out8( 0x71, value );
}
