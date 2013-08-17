#ifndef _IOPort_h_
#define _IOPort_h_ 1

#include "mach/IOBase.h"
#include "mach/platform.h"

class IOPort : public IOBase {
  IOPort(const IOPort&) = delete;
  IOPort& operator=(const IOPort&) = delete;

  uint16_t ioPort;
  mword portSize;
  const char* portName;
public:
  IOPort(const char* name) : ioPort(0), portSize(0), portName(name) {}
  ~IOPort() { free(); }

  mword size() const {
    return portSize;
  }
  uint16_t base() const {
    return ioPort;
  }
  const char* name() const {
    return portName;
  }
  operator bool() const {
    return portSize != 0;
  }

  uint8_t read8(mword offset = 0);
  uint16_t read16(mword offset = 0);
  uint32_t read32(mword offset = 0);
  uint64_t read64(mword offset = 0);
  void write8(uint8_t val, mword offset = 0);
  void write16(uint16_t val, mword offset = 0);
  void write32(uint32_t val, mword offset = 0);
  void write64(uint64_t val, mword offset = 0);

  bool allocate(uint16_t port, mword size);
  void free();
};

#endif /* _IOPort_h_ */
