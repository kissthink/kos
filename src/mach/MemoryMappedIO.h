#ifndef _MemoryMappedIO_h_
#define _MemoryMappedIO_h_ 1

#include "mach/IOBase.h"
#include "mach/MemoryRegion.h"

class MemoryMappedIO : public IOBase, public MemoryRegion {
  MemoryMappedIO(const MemoryMappedIO&) = delete;
  MemoryMappedIO& operator=(const MemoryMappedIO&) = delete;

  mword ioOffset;
  mword ioPadding;

public:
  MemoryMappedIO(const char* name, mword offset, mword padding)
    : IOBase(), MemoryRegion(name), ioOffset(offset), ioPadding(padding) {}
  virtual ~MemoryMappedIO() {}

  mword size() const {
    return MemoryRegion::size();
  }
  uint8_t read8(mword offset = 0) {
    return *reinterpret_cast<volatile uint8_t*>(adjust_pointer(virtualAddress(), (offset*ioPadding)+ioOffset));
  }
  uint16_t read16(mword offset = 0) {
    return *reinterpret_cast<volatile uint16_t*>(adjust_pointer(virtualAddress(), (offset*ioPadding)+ioOffset));
  }
  uint32_t read32(mword offset = 0) {
    return *reinterpret_cast<volatile uint32_t*>(adjust_pointer(virtualAddress(), (offset*ioPadding)+ioOffset));
  }
  uint64_t read64(mword offset = 0) {
    return *reinterpret_cast<volatile uint64_t*>(adjust_pointer(virtualAddress(), (offset*ioPadding)+ioOffset));
  }
  void write8(uint8_t val, mword offset = 0) {
    *reinterpret_cast<volatile uint8_t*>(adjust_pointer(virtualAddress(), (offset*ioPadding)+ioOffset)) = val;
  }
  void write16(uint16_t val, mword offset = 0) {
    *reinterpret_cast<volatile uint16_t*>(adjust_pointer(virtualAddress(), (offset*ioPadding)+ioOffset)) = val;
  }
  void write32(uint32_t val, mword offset = 0) {
    *reinterpret_cast<volatile uint32_t*>(adjust_pointer(virtualAddress(), (offset*ioPadding)+ioOffset)) = val;
  }
  void write64(uint64_t val, mword offset = 0) {
    *reinterpret_cast<volatile uint64_t*>(adjust_pointer(virtualAddress(), (offset*ioPadding)+ioOffset)) = val;
  }
  operator bool() const {
    return MemoryRegion::operator bool();
  }
};

#endif /* _MemoryMappedIO_h_ */
