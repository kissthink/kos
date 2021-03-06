#ifndef NonBlockSemaphore_h_
#define NonBlockSemaphore_h_ 1

#include "ipc/SpinLock.h"
#include <atomic>

/**
 * Non-blocking semaphore used by gdb code.
 */
class NonBlockSemaphore {
  SpinLock lk;
  atomic<int> counter;
public:
  NonBlockSemaphore(int counter = 0): counter(counter) {}
  ptr_t operator new(size_t) { return ::operator new(NonBlockSemaphore::size()); }
  void operator delete(ptr_t ptr) { globaldelete(ptr, NonBlockSemaphore::size()); }
  static constexpr size_t size() {
    return sizeof(SpinLock) < sizeof(vaddr) ? sizeof(vaddr) : sizeof(NonBlockSemaphore);
  }
  void P() volatile {
    for(;;) {
      Pause();
      ScopedLock<> so(lk);
      if (counter <= 0) {
        continue;
      } else {
        counter--;
        break;
      }
    }
  }

  void V() volatile {
    ScopedLock<> so(lk);
    counter += 1;
  }

  void resetCounter(int newCounter) {
    ScopedLock<> so(lk);
    counter = newCounter;
  }
};

#endif // NonBlockSemaphore_h_
