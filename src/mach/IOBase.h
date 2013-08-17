#ifndef _IOBase_h_
#define _IOBase_h_ 1

#include "mach/platform.h"

class IOBase {
  IOBase(const IOBase&) = delete;
  IOBase& operator=(const IOBase&) = delete;
protected:
  IOBase() {}
public:
  virtual ~IOBase() {}
  virtual mword size() const = 0;
  virtual uint8_t read8(mword offset = 0) = 0;
  virtual uint16_t read16(mword offset = 0) = 0;
  virtual uint32_t read32(mword offset = 0) = 0;
  virtual uint64_t read64(mword offset = 0) = 0;
  virtual void write8(uint8_t value, mword offset = 0) = 0;
  virtual void write16(uint16_t value, mword offset = 0) = 0;
  virtual void write32(uint32_t value, mword offset = 0) = 0;
  virtual void write64(uint64_t value, mword offset = 0) = 0;
  virtual operator bool() const = 0;  // check whether this class is usable
};

#endif /* _IOBase_h_ */
