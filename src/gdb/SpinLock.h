#ifndef GdbSpinLock_h_
#define GdbSpinLock_h_ 1

#include "kern/globals.h"

/**
 * Only used inside GDB stub's interrupt handler.
 * Prevents Scheduler from throwing KASSERT on lock count.
 */
namespace gdb {

class SpinLock {
  bool locked;
  void acquireInternal() volatile {
    while (__atomic_test_and_set(&locked, __ATOMIC_SEQ_CST))
      while (locked) Pause();
  }
  void releaseInternal() volatile {
    __atomic_clear(&locked, __ATOMIC_SEQ_CST);
  }
public:
  SpinLock() : locked(false) {}
  ptr_t operator new(std::size_t) { return ::operator new(SpinLock::size()); }
  void operator delete(ptr_t ptr) { globaldelete(ptr, SpinLock::size()); }
  static constexpr size_t size() {
    return sizeof(SpinLock) < sizeof(vaddr) ? sizeof(vaddr) : sizeof(SpinLock);
  }
  void acquireISR() volatile {
    KASSERT(!Processor::interruptsEnabled(), "interrupt must be disabled");
    acquireInternal();
  }
  void releaseISR() volatile {
    KASSERT(!Processor::interruptsEnabled(), "interrupt must be disabled");
    releaseInternal();
  }
};

} // namespace gdb

#endif // GdbSpinLock_h_
