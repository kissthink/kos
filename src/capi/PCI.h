#ifndef _C_PCI_h_
#define _C_PCI_h_

#ifdef __cplusplus
#include <cstdint>    // do not include stdint.h for C include because linux header defines it
extern "C" {
#endif

  uint32_t PCIReadConfigWord(uint16_t bus, uint16_t dev, uint16_t func, uint16_t reg, uint32_t width);
  void PCIWriteConfigWord(uint16_t bus, uint16_t dev, uint16_t func, uint16_t reg, uint32_t width, uint32_t value);
  uint32_t PCIProbeDevice(uint16_t bus, uint16_t device);
  void PCIProbeAll();

#ifdef __cplusplus
}
#endif

#endif /* _C_PCI_h_ */
