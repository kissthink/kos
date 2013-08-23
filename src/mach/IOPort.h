#ifndef _IOPort_h_
#define _IOPort_h_ 1

#include "mach/platform.h"

class IOPort {
  friend class IOPortManager; // only modifiable via this class

  IOPort(const IOPort&) = delete;
  IOPort& operator=(const IOPort&) = delete;

  uint16_t baseAddr;
  size_t portRange;
  const char* name;

public:
  IOPort(const char* name) : baseAddr(0), portRange(0), name(name) {}
  ~IOPort() { releaseMe(); }

  uint16_t base() const {
    return baseAddr;
  }
  size_t range() const {
    return portRange;
  }
  const char* getName() const {
    return name;
  }
  void releaseMe();
};

#endif /* _IOPort_h_ */
