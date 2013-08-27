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
  StdOut.outln("[2] Write to a hard disk");
  StdOut.outln("[3] Read the hard disk");
  StdOut.outln("[q] Exit");
  StdOut.outln("");
}

extern void cdi_net_send(int, ptr_t, size_t);
struct cdi_storage_device;
extern int cdi_storage_read(cdi_storage_device* device, uint64_t pos, size_t size, ptr_t dest);
extern int cdi_storage_write(cdi_storage_device* device, uint64_t pos, size_t size, ptr_t src);

void Drivers::handleKey(char key) {
  if (!running) return;
  switch (key) {
    case '1':
      cdi_net_send(0, (ptr_t)"hello world!", 13);
      break;
    case '2': {
      int res = cdi_storage_write(0, 0, 13, (ptr_t)"hello world!");
      DBG::outln(DBG::Basic, "write ", (res == 0 ? "success" : "failed"));
    } break;
    case '3': {
      char buf[256] = { 0 };
      int res = cdi_storage_read(0, 0, 13, &buf);
      DBG::outln(DBG::Basic, "read ", (res == 0 ? "success" : "failed"));
      StdOut.outln(buf);
    } break;
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
