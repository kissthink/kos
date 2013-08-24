#ifndef _IOPort_h_
#define _IOPort_h_ 1

#include "mach/platform.h"
#include "util/Output.h"

class IOPort {
  friend class IOPortManager; // only modifiable via this class

  IOPort(const IOPort&) = delete;
  IOPort& operator=(const IOPort&) = delete;

  uint16_t baseAddr;
  size_t portRange;
  const char* name;
  bool initialized;

  void releaseMe();
public:
  IOPort(const char* name) : baseAddr(0), portRange(0), name(name), initialized(false) {}
  ~IOPort() { releaseMe(); }

  inline uint16_t base() const {
    return baseAddr;
  }
  inline size_t range() const {
    return portRange;
  }
  inline const char* getName() const {
    return name;
  }

  // I/O
  inline uint8_t read8(uint8_t offset) {
    KASSERT0( initialized );
    KASSERT1( offset < portRange, offset );
    return in8( baseAddr + offset );
  }
  inline uint16_t read16(uint8_t offset) {
    KASSERT0( initialized );
    KASSERT1( offset < portRange, offset );
    return in16( baseAddr + offset );
  }
  inline uint32_t read32(uint8_t offset) {
    KASSERT0( initialized );
    KASSERT1( offset < portRange, offset );
    return in32( baseAddr + offset );
  }
  inline void write8(uint8_t offset, uint8_t val) {
    KASSERT0( initialized );
    KASSERT1( offset < portRange, offset );
    out8( baseAddr + offset, val );
  }
  inline void write16(uint8_t offset, uint16_t val) {
    KASSERT0( initialized );
    KASSERT1( offset < portRange, offset );
    out16( baseAddr + offset, val );
  }
  inline void write32(uint8_t offset, uint32_t val) {
    KASSERT0( initialized );
    KASSERT1( offset < portRange, offset );
    out32( baseAddr + offset, val );
  }
};

#endif /* _IOPort_h_ */
