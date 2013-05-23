#ifndef GdbSpinLock_h_
#define GdbSpinLock_h_ 1

#include "kern/globals.h"

/**
 * Only used inside GDB stub's interrupt handler.
 * Prevents Scheduler from throwing KASSERT on lock count.
 */
namespace gdb {

class GdbSpinLock {
  bool locked;
  void acquireInternal() volatile {
    while (__atomic_test_and_set(&locked, __ATOMIC_SEQ_CST))
      while (locked) Pause();
  }
  void releaseInternal() volatile {
    __atomic_clear(&locked, __ATOMIC_SEQ_CST);
  }
public:
  GdbSpinLock() : locked(false) {}
  ptr_t operator new(std::size_t) { return ::operator new(GdbSpinLock::size()); }
  void operator delete(ptr_t ptr) { globaldelete(ptr, GdbSpinLock::size()); }
  static constexpr size_t size() {
    return sizeof(GdbSpinLock) < sizeof(vaddr) ? sizeof(vaddr) : sizeof(GdbSpinLock);
  }
  void acquireISR() volatile {
    KASSERT0(!Processor::interruptsEnabled());
    acquireInternal();
  }
  void releaseISR() volatile {
    KASSERT0(!Processor::interruptsEnabled());
    releaseInternal();
  }
};

} // namespace gdb

#endif // GdbSpinLock_h_
