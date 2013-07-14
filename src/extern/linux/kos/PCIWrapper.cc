#include "extern/linux/kos/PCI.h"
#include "mach/PCI.h"

uint32_t PCIReadConfigWord(uint16_t bus, uint16_t dev, uint16_t func, uint16_t reg, uint32_t width) {
  return PCI::readConfigWord(bus, dev, func, reg, width);
}

void PCIWriteConfigWord(uint16_t bus, uint16_t dev, uint16_t func, uint16_t reg, uint32_t width, uint32_t value) {
  PCI::writeConfigWord(bus, dev, func, reg, width, value);
}

uint32_t PCIProbeDevice(uint16_t bus, uint16_t device) {
  return PCI::probeDevice(bus, device);
}

void PCIProbeAll() {
  PCI::probeAll();
}
