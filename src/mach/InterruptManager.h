#ifndef _Interrupt_Manager_h_
#define _Interrupt_Manager_h_ 1

#include "ipc/SpinLock.h"
#include "mach/platform.h"
#include "util/basics.h"

class InterruptHandler;
class InterruptHandlerWithError;

class InterruptManager {
#if 0 // couldn't figure out friending extern "C" function
  friend void isr_handler_gen(mword num);
  friend void isr_handler_gen_err(mword num, mword errcode);
#endif

  InterruptManager() = delete;
  InterruptManager(const InterruptManager&) = delete;
  InterruptManager& operator=(const InterruptManager&) = delete;


  static SpinLock lk;
  static InterruptHandler* genHandlers[256];
  static InterruptHandlerWithError* errHandlers[256];

public:
  static bool registerHandler(mword vector, InterruptHandler* handler);
  static bool registerHandler(mword vector, InterruptHandlerWithError* handler);
  static bool unregisterHandler(mword vector);
  static void init();

  static void handleInterrupt(mword vector);
  static void handleInterrupt(mword vector, mword errcode);
};

#endif /* _Interrupt_Manager_h */
