#ifndef Semaphore_h_
#define Semaphore_h_ 1

#include "util/SpinLock.h"
#include <atomic>

/**
 * Non-blocking semaphore used by gdb code.
 */
namespace gdb {

class Semaphore {
  SpinLock lk;
  std::atomic<int> counter;
public:
  Semaphore(int counter = 0): counter(counter) {}
  ptr_t operator new(std::size_t) { return ::operator new(Semaphore::size()); }
  void operator delete(ptr_t ptr) { globaldelete(ptr, Semaphore::size()); }
  static constexpr size_t size() {
    return sizeof(SpinLock) < sizeof(vaddr) ? sizeof(vaddr) : sizeof(Semaphore);
  }
  void P() volatile {
    for(;;) {
      Pause();
      ScopedLockISR<> so(lk);
      if (counter <= 0) {
        continue;
      } else {
        counter--;
        break;
      }
    }
  }

  void V() volatile {
    ScopedLockISR<> so(lk);
    counter += 1;
  }

  void resetCounter(int newCounter) {
    ScopedLockISR<> so(lk);
    counter = newCounter;
  }
};

} // namespace gdb

#endif // Semaphore_h_
