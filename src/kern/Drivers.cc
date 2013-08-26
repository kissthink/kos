#include "util/basics.h"
#include "kern/Drivers.h"
#include "kern/Debug.h"
#include "util/Output.h"

Semaphore Drivers::waitForKey;
bool Drivers::running = false;

void Drivers::showMenu() {
  StdOut.outln("Drivers test menu");
  StdOut.outln("Choose following options:");
  StdOut.outln("[1] Send a network packet");
  StdOut.outln("[q] Exit");
  StdOut.outln("");
}

extern void cdi_net_send(int, ptr_t, size_t);

void Drivers::handleKey(char key) {
  if (!running) return;
  switch (key) {
    case '1':
      StdErr.outln("Sending a network packet");
      cdi_net_send(0, (ptr_t)"hello world!", 13);
      break;
    case 'q':
      StdErr.outln("Exiting drivers test...");
      running = false;
      break;
    default:
      StdErr.outln("Key ", key, " pressed"); 
      break;
  }
  waitForKey.V();
}

void Drivers::runTest() {
  running = true;
  for (;;) {
    showMenu();
    waitForKey.P();
    if (!running) break;
  }
}
