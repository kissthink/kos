#ifndef _Drivers_h_
#define _Drivers_h_ 1

#include "ipc/BlockingSync.h"
#include "ipc/SyncQueues.h"
#include "dev/Keyboard.h"

class Command;
class Drivers {
  Drivers() = delete;
  Drivers(const Drivers&) = delete;
  Drivers& operator=(const Drivers&) = delete;

  static Thread* testThread;
  static ProdConsQueue<StaticRingBuffer<Command*,16>> testQueue;
  static void showMenu();
  static Semaphore waitForKey;
  static volatile bool running;

  static void run(ptr_t arg);
public:
  static void parseCommands(Keyboard& keyboard, char key);
  static void runTest();
};

#endif /* _Drivers_h_ */
