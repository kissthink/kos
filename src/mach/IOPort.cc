#include "mach/IOPort.h"
#include "mach/IOPortManager.h"
#include "util/Output.h"

uint8_t IOPort::read8(mword offset) {
  uint8_t val;
  asm volatile("inb %%dx, %%al" : "=a"(val) : "dN"(ioPort + offset));
  return val;
}

uint16_t IOPort::read16(mword offset) {
  uint16_t val;
  asm volatile("inw %%dx, %%ax" : "=a"(val) : "dN"(ioPort + offset));
  return val;
}

uint32_t IOPort::read32(mword offset) {
  uint32_t val;
  asm volatile("in %%dx, %%eax" : "=a"(val) : "dN"(ioPort + offset));
  return val;
}

uint64_t IOPort::read64(mword offset) {
  ABORT1("unimplemented");
  return 0;
}

void IOPort::write8(uint8_t val, mword offset) {
  asm volatile("outb %%al, %%dx" :: "dN"(ioPort + offset), "a"(val));
}

void IOPort::write16(uint16_t val, mword offset) {
  asm volatile("outw %%ax, %%dx" :: "dN"(ioPort + offset), "a"(val));
}

void IOPort::write32(uint32_t val, mword offset) {
  asm volatile("out %%eax, %%dx" :: "dN"(ioPort + offset), "a"(val));
}

void IOPort::write64(uint64_t val, mword offset) {
  ABORT1("unimplemented");
}

bool IOPort::allocate(uint16_t port, mword size) {
  if (portSize) free(); // free any allocated I/O ports
  if (IOPortManager::allocate(this, port, size)) {
    ioPort = port;
    portSize = size;
    return true;
  }
  return false;
}

void IOPort::free() {
  if (portSize) {
    IOPortManager::free(this);
    ioPort = 0;
    portSize = 0;
  }
}

