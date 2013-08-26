#ifndef _Drivers_h_
#define _Drivers_h_ 1

#include "ipc/BlockingSync.h"

class Drivers {
  Drivers() = delete;
  Drivers(const Drivers&) = delete;
  Drivers& operator=(const Drivers&) = delete;

  static void showMenu();
  static Semaphore waitForKey;
  static bool running;
public:
  static void handleKey(char key);
  static void runTest();
};

#endif /* _Drivers_h_ */
