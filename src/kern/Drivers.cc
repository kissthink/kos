#include "util/basics.h"
#include "kern/Drivers.h"
#include "kern/Debug.h"
#include "util/Output.h"
#include "dev/Keyboard.h"

Thread* Drivers::testThread;
ProdConsQueue<StaticRingBuffer<Command*,16>> Drivers::testQueue;
Semaphore Drivers::waitForKey;
volatile bool Drivers::running = false;

void Drivers::showMenu() {
  StdOut.outln("Drivers test menu");
  StdOut.outln("Choose following options:");
  StdOut.outln("[1] Send a network packet");
  StdOut.outln("[2] Write to a hard disk");
  StdOut.outln("[3] Read the hard disk");
  StdOut.outln("[4] Write to floppy disk");
  StdOut.outln("[5] Read from floppy disk");
  StdOut.outln("[6] Send 1000 network packets");
  StdOut.outln("[7] Read/Write to hard disk 1000 times");
  StdOut.outln("[8] Read/Write to floppy disk 1000 times");
  StdOut.outln("[q] Exit");
  StdOut.outln("");
}

extern void cdi_net_send(int, ptr_t, size_t);
extern void cdi_net_send(ptr_t, size_t);
extern int cdi_storage_read(const kstring& devName, uint64_t pos, size_t size, ptr_t dest);
extern int cdi_storage_write(const kstring& devName, uint64_t pos, size_t size, ptr_t src);

static kstring hdiskName = "ata00";
static kstring fdiskName = "fd0";

static Keyboard::KeyCode getNextKey(Keyboard& keyboard) {
  Keyboard::KeyCode key = keyboard.read();
  StdErr.out((char) key);
  return key;
}

class Command {
  char buf[256];
  size_t len;
public:
  Command(Keyboard& keyboard) : len(0) {
    len = 0;
    memset(buf, 0, 256);
    Keyboard::KeyCode nextKey;
    while ((nextKey = getNextKey(keyboard)) != 0x0d && len < 255) {
      buf[len++] = (char) nextKey;
    }
  }
  virtual ~Command() {}
  virtual bool run() = 0;
  char* getBuffer() { return buf; }
  size_t getLength() const { return len; }
};

struct SendNetworkPacketTest : public Command {
  SendNetworkPacketTest(Keyboard& keyboard) : Command(keyboard) {}
  bool run() {
    cdi_net_send((ptr_t) getBuffer(), getLength());
    DBG::outln(DBG::Basic, "packet sent");
    return true;
  }
};

struct WriteToHardDiskTest : public Command {
  WriteToHardDiskTest(Keyboard& keyboard) : Command(keyboard) {}
  bool run() {
    int res = cdi_storage_write(hdiskName, 0, 256, (ptr_t) getBuffer());
    DBG::outln(DBG::Basic, "write ", (res == 0 ? "success" : "failed"));
    return res == 0 ? true : false;
  }
};

struct ReadFromHardDiskTest : public Command {
  ReadFromHardDiskTest(Keyboard& keyboard) : Command(keyboard) {}
  bool run() {
    int res = cdi_storage_read(hdiskName, 0, 256, getBuffer());
    if (res == 0) {
      StdOut.outln(getBuffer());
    } else {
      DBG::outln(DBG::Basic, "read failed");
    }
    return res == 0 ? true : false;
  }
};

struct WriteToFloppyDiskTest : public Command {
  WriteToFloppyDiskTest(Keyboard& keyboard) : Command(keyboard) {}
  bool run() {
    int res = cdi_storage_write(fdiskName, 0, 256, (ptr_t) getBuffer());
    DBG::outln(DBG::Basic, "write ", (res == 0 ? "success" : "failed"));
    return res == 0 ? true : false;
  }
};

struct ReadFromFloppyDiskTest : public Command {
  ReadFromFloppyDiskTest(Keyboard& keyboard) : Command(keyboard) {}
  bool run() {
    int res = cdi_storage_read(fdiskName, 0, 256, getBuffer());
    if (res == 0) {
      StdOut.outln(getBuffer());
    } else {
      DBG::outln(DBG::Basic, "read failed");
    }
    return res == 0 ? true : false;
  }
};

struct Send1000NetworkPacketTest : public Command {
  Send1000NetworkPacketTest(Keyboard& keyboard) : Command(keyboard) {}
  bool run() {
    for (int i = 0; i < 1000; i++) {
      cdi_net_send((ptr_t) getBuffer(), getLength());
      DBG::outln(DBG::Basic, "packet sent");
    }
    return true;
  }
};

struct ReadWriteHardDiskStressTest : public Command {
  ReadWriteHardDiskStressTest(Keyboard& keyboard) : Command(keyboard) {}
  bool run() {
    for (int i = 0; i < 1000; i++) {
      int res = cdi_storage_write(hdiskName, 0, 256, (ptr_t) getBuffer());
      KASSERT0( res == 0 );
      res = cdi_storage_read(hdiskName, 0, 256, getBuffer());
      KASSERT0( res == 0 );
    }
    return true;
  }
};

struct ReadWriteFloppyDiskStressTest : public Command {
  ReadWriteFloppyDiskStressTest(Keyboard& keyboard) : Command(keyboard) {}
  bool run() {
    for (int i = 0; i < 1000; i++) {
      int res = cdi_storage_write(fdiskName, 0, 256, (ptr_t) getBuffer());
      KASSERT0( res == 0 );
      res = cdi_storage_read(fdiskName, 0, 256, getBuffer());
      KASSERT0( res == 0 );
    }
    return true;
  }
};

void Drivers::run(ptr_t arg) {
  while (running) {
    Command* cmd = testQueue.remove();
    cmd->run();
    delete cmd;
  }
}

void Drivers::parseCommands(Keyboard& keyboard, char key) {
  if (!running) return;
  switch (key) {
    case '1': {
      Command* cmd = new SendNetworkPacketTest(keyboard);
      testQueue.append(cmd);
    } break;
    case '2': {
      Command* cmd = new WriteToHardDiskTest(keyboard);
      testQueue.append(cmd);
    } break;
    case '3': {
      Command* cmd = new ReadFromHardDiskTest(keyboard);
      testQueue.append(cmd);
    } break;
    case '4': {
      Command* cmd = new WriteToFloppyDiskTest(keyboard);
      testQueue.append(cmd);
    } break;
    case '5': {
      Command* cmd = new ReadFromFloppyDiskTest(keyboard);
      testQueue.append(cmd);
    } break;
    case '6': {
      Command* cmd = new Send1000NetworkPacketTest(keyboard);
      testQueue.append(cmd);
    } break;
    case '7': {
      Command* cmd = new ReadWriteHardDiskStressTest(keyboard);
      testQueue.append(cmd);
    } break;
    case '8': {
      Command* cmd = new ReadWriteFloppyDiskStressTest(keyboard);
      testQueue.append(cmd);
    } break;
    case 'q': {
      StdErr.outln("Exiting drivers test...");
      running = false;
    } break;
    default: break;
  }

  waitForKey.V(); // ready for next round
}

void Drivers::runTest() {
  running = true;
  testThread = Thread::create(kernelSpace, "drivers test");
  kernelScheduler.run(*testThread, run, nullptr);

  for (;;) {
    showMenu();
    waitForKey.P();
    if (!running) break;
  }
}
