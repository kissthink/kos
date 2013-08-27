#include "util/basics.h"
#include "kern/Drivers.h"
#include "kern/Debug.h"
#include "util/Output.h"
#include "dev/Keyboard.h"

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

static Keyboard::KeyCode getNextKey(Keyboard& keyboard) {
  Keyboard::KeyCode key = keyboard.read();
  StdErr.out((char) key);
  return key;
}

void Drivers::handleKey(Keyboard& keyboard, char key) {
  if (!running) return;
  char buf[256] = { 0 };  // assuming 256 bytes is enough for testing
  Keyboard::KeyCode nextKey;
  switch (key) {
    case '1': {
      int i = 0;
      while ((nextKey = getNextKey(keyboard)) != 0x0d && i < 255) {
        buf[i++] = (char) nextKey;
      }
      cdi_net_send(0, (ptr_t) buf, i);
    } break;
    case '2': {
      int i = 0;
      memset(buf, 0, 256);
      while ((nextKey = getNextKey(keyboard)) != 0x0d && i < 255) {
        buf[i++] = (char) nextKey;
      }
      int res = cdi_storage_write(0, 0, 256, (ptr_t) buf);
      DBG::outln(DBG::Basic, "write ", (res == 0 ? "success" : "failed"));
    } break;
    case '3': {
      memset(buf, 0, 256);
      int res = cdi_storage_read(0, 0, 256, &buf);
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
