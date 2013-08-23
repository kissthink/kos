#include "cdi/misc.h"

void cdi_register_irq(uint8_t irq, void (*handler)(cdi_device *),
                      cdi_device* device) {
  // TODO
}

int cdi_reset_wait_irq(uint8_t irq) {
  // TODO
  return -1;
}

int cdi_wait_irq(uint8_t irq, uint32_t timeout) {
  // TODO
  return -1;
}

int cdi_ioports_alloc(uint16_t start, uint16_t count) {
  // TODO
  return -1;
}

int cdi_ioports_free(uint16_t start, uint16_t count) {
  // TODO
  return -1;
}

void cdi_sleep_ms(uint32_t ms) {
  // TODO
}
