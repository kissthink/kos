#ifndef _Delay_h_
#define _Delay_h_ 1

#include "mach/Calibrate.h"
#include "mach/CPU.h"
#include "mach/Processor.h"
#include "mach/platform.h"

// from linux include/asm-generic/delay.h and arch/x86/delay.c
class Delay {
  static void delay(mword loops) {
    mword end = CPU::readTSC() + loops;
    while (CPU::readTSC() < end) Pause();
#if 0
    asm volatile(
      "test %0,%0     \n"
      " jz 3f         \n"
      " jmp 1f        \n"
      ".align 16      \n"
      "1: jmp 2f      \n"
      ".align 16      \n"
      "2: dec %0      \n"
      " jnz 2b        \n"
      "3: dec %0      \n"
      :: "a"(loops)
    );
#endif
  }
  static inline void __udelay(mword usecs) {
    int d0;
//    DBG::outln(DBG::Basic, "delaying for ", usecs, " usecs");
    usecs *= 4;
    asm("mull %%edx"
      :"=d"(usecs), "=&a"(d0)
      :"1"(usecs), "0"
      (Calibrate::getCyclesPerTick() * 250));
//    DBG::outln(DBG::Basic, "cycles/tick", Calibrate::getCyclesPerTick(), " cycles");
//    DBG::outln(DBG::Basic, "delaying really for ", usecs, " cycles");
    Processor::incLockCount();
    delay(++usecs);
    Processor::decLockCount();
  }
  Delay() = delete;
  Delay(const Delay& d) = delete;
  Delay& operator=(const Delay& d) = delete;
public:
  static __attribute__ ((noinline)) void udelay(mword usecs) {
    KASSERT1(usecs / 20000 < 1, "usecs too large!");
    __udelay(usecs * 0x000010c7);   // 2**32 / 1000000
  }
  static __attribute__ ((noinline)) void ndelay(mword nsecs) {
    KASSERT1(nsecs / 20000 < 1, "nsecs too large!");
    __udelay(nsecs * 0x00005);      // 2**32 / 1000000000
  }
};

#endif /* _Delay_h_ */
