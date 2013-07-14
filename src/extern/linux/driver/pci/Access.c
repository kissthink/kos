#include "linux/pci.h"
#include "PCI.h"

/**
 * Read a byte from the configuration space of a specified PCI bus. The where argument
 * is the byte offset from the beginning of the configuration space. The value fetched
 * from configuration space is returned through the val pointer, and the return value
 * of the function is an error code. The word and dword functions convert the value just
 * read from little-endian to the native byte order of the processor.
 */
int pci_bus_read_config_byte(struct pci_bus *bus, unsigned int devfn, int where, u8 *value) {
  int i, res = 0;
  uint16_t reg = where >> 2;    // convert byte offset into register number
  uint32_t val = PCIReadConfigWord(bus->number, 0, devfn, reg, 32);
  uint32_t mask = ((1) << 8) - 1;
  uint16_t offset = where % 4;
  for (i = 0; i < offset; i++) {
    mask <<= 8;
  }
  val = val & mask;
  for (i = 0; i < offset; i++) {
    val >>= 8;
  }
  *value = (u8) val;
  return res;
}

int pci_bus_read_config_word(struct pci_bus *bus, unsigned int devfn, int where, u16 *value) {
  int res = 0;
  u8 val1 = 0, val2 = 0;
  res = pci_bus_read_config_byte(bus, devfn, where, &val1);
  res = pci_bus_read_config_byte(bus, devfn, where+1, &val2);
  *value = (val2 << 8 | val1);
  return res;
}

int pci_bus_read_config_dword(struct pci_bus *bus, unsigned int devfn, int where, u32 *value) {
  int res = 0;
  u8 val1 = 0, val2 = 0, val3 = 0, val4 = 0;
  res = pci_bus_read_config_byte(bus, devfn, where, &val1);
  res = pci_bus_read_config_byte(bus, devfn, where+1, &val2);
  res = pci_bus_read_config_byte(bus, devfn, where+2, &val3);
  res = pci_bus_read_config_byte(bus, devfn, where+3, &val4);
  *value = (val4 << 24 | val3 << 16 | val2 << 8 | val1);
  return res;
}
