#include <cstring>
#include <cstdarg>
#include <cstdint>

#include "dev/Serial.h"
#include "gdb/GdbCpu.h"
#include "gdb/Gdb.h"
#include "gdb/gdb_asm_functions.h"
#include "gdb/NonBlockSemaphore.h"
#include "gdb/VContAction.h"
#include "gdb/List.h"
#include "kern/Thread.h"
#include "mach/Machine.h"
#include "mach/Processor.h"
#include "kern/Debug.h"
#include "util/EmbeddedQueue.h"

#undef __STRICT_ANSI__
#include <cstdio>

// function stubs for Gdb remote serial protocol
void putDebugChar(unsigned char ch) {
  SerialDevice0::write(ch);
}
int getDebugChar() {
  return SerialDevice0::read();
}
void exceptionHandler(int exceptionNumber, void (*exceptionAddr)()) {
  Machine::setupIDT(exceptionNumber, reinterpret_cast<vaddr>(exceptionAddr));
}

class BreakPoint {
  mword addr;
  char opcode;
public:
  BreakPoint(): addr(0), opcode(0) {}
  BreakPoint(mword addr, char opCode): addr(addr), opcode(opCode) {}
  void set(mword a, char op) { addr = a; opcode = op; }
  char getOpcode() const { return opcode; }
  bool operator==(const BreakPoint& bp) const {
    return addr == bp.addr;
  }
};
static List<BreakPoint> bpList;             // tracks breakpoints inserted from gdb's requests

#define BREAKPOINT() asm("int $3");
extern "C" void handle_exception(int64_t);
static const char hexchars[]="0123456789abcdef";
static const int BUFMAX = 8096;             // in/out buffer for communication with gdb frontend
                                            // XXX be sure to let gdb frontend know when this changes
static char initialized;                    // Gdb initialized?
static atomic<bool> isBatonFree(true);      // is baton free to be picked up?
static atomic<int> lockHolder(-1);          // who is holding the baton?
static bool* waiting;                       // represents a CPU waiting on entry queue.
static bool* shouldReturnFromException;     // return from interrupt handler right away (allstop)
static NonBlockSemaphore entry_q;           // entry/exit protocol
static GdbCpu** gCpu = nullptr;             // cpu states used exclusively for 'g' operations
static EmbeddedQueue<VContAction> vContActionQueue;   // vCont packets are queued here
static VContAction* prevAction = nullptr;   // previous vContAction
static bool allstop = false;                // allstop mode
void *mem_fault_return_addr = nullptr;      // where to return from fault handler?
static bool* preempt;                       // tracks whether preemption was disabled by gdb
static bool breakOnUnwindDebugHook = false; // detect if breakpoint was inserted on _Unwind_DebugHook

// current thread ID to 1-based
static int getCurrCpuId() {
  return Processor::getApicID()+1;
}

// GCpu is used for querying specific CPU by gdb
static void setGCpu(int cpuIdx) {
  int currCpuIdx = Processor::getApicID();
  gCpu[currCpuIdx] = Gdb::getCpu(cpuIdx);
}
static GdbCpu* getGCpu() {
  int cpuIdx = Processor::getApicID();
  if (gCpu[cpuIdx] == nullptr) setGCpu(cpuIdx);
  return gCpu[cpuIdx];
}
// disable preemption for step/next will prevent
// the current thread from migrating to another CPU
static void disablePreemption() {
  preempt[Processor::getApicID()] = false;
  Processor::incLockCount();
}
static void enablePreemption() {
  if (!preempt[Processor::getApicID()]) {
    Processor::decLockCount();
    preempt[Processor::getApicID()] = true;
  }
}
// baton available if nobody has it or I am holding it
static bool canGrabBaton(int threadId) {
  if (isBatonFree) return true;
  if (lockHolder == threadId) return true;
  return false;
}
// use this before returning from step/next/continue
static void scheduleVContAction(bool isContinue = false) {
  vContActionQueue.front()->executed = true;
  if (!isContinue) disablePreemption();
}

// internal method for _returnFromException
// set CPU state to running, and wake up all other threads if in allstop
void __returnFromException() {
  Gdb::setCpuState(cpuState::RUNNING);
  if (allstop) {
    int threadId = Processor::getApicID();
    for (int i = 0; i < Gdb::getNumCpusInitialized(); i++) {
      if (i == threadId) continue;
      if (waiting[i]) {         // wake up all waiting threads
        waiting[i] = false;
        shouldReturnFromException[i] = true;
        Gdb::V(i);
      }
    }
  }
}
void _returnFromException() {
  __returnFromException();
//  entry_q.releaseISR();
  restoreRegisters(Gdb::getCurrentCpu());   // restore back to current CPU's stored state
}
void _returnFromExceptionLocked() {         // called with entry_q locked (exit protocol)
  __returnFromException();
  entry_q.V();
  restoreRegisters(Gdb::getCurrentCpu());   // restore back to current CPU's stored state
}

// hex character to integer
int hex(char ch) {
  if ((ch >= 'a') && (ch <= 'f')) return (ch - 'a' + 10);
  if ((ch >= '0') && (ch <= '9')) return (ch - '0');
  if ((ch >= 'A') && (ch <= 'F')) return (ch - 'A' + 10);
  return -1;
}

// Routines for gdb frontend communication
static char inputBuffer[BUFMAX];    // buffers for communication
static char outputBuffer[BUFMAX];

// scan for the sequence $<data>#<checksum>
char* getpacket() {
  char *buffer = &inputBuffer[0];
  char checksum, xmitcsum, ch;
  int count;

  while (true) {
    while ((ch = getDebugChar()) != '$');             // find start of packet
retry:
    checksum = 0, xmitcsum = -1, count = 0;
    while (count < BUFMAX - 1) {                      // read to end of packet '#' or buffer
      ch = getDebugChar();
      if (ch == '$') goto retry;
      if (ch == '#') break;
      checksum += ch;
      buffer[count] = ch;
      count++;
    }
    buffer[count] = 0;
    if (ch == '#') {                                  // found '#' now validate checksum
      ch = getDebugChar();
      xmitcsum = hex(ch) << 4;
      ch = getDebugChar();
      xmitcsum += hex(ch);                            // xmitsum is computed checksum
      if (checksum != xmitcsum) putDebugChar ('-');   // failed checksum
      else {
        putDebugChar ('+');                           // success
        if (buffer[2] == ':') {                       // if sequence char present, reply seq ID
          putDebugChar(buffer[0]);
          putDebugChar(buffer[1]);
          return &buffer[3];
        }
        return &buffer[0];
      } // if
    } // if
  } // while
}
// send the packet in buffer
void putpacket(char *buffer) {
  unsigned char checksum;
  int count;
  char ch;

  // $<packet info>#<checksum>.
  do {
    putDebugChar('$');                        // start of packet
    checksum = 0, count = 0;
    while ( (ch = buffer[count]) ) {
      putDebugChar(ch);
      checksum += ch;
      count += 1;
    }
    putDebugChar('#');                        // end of data
    putDebugChar(hexchars[checksum >> 4]);    // write checksum
    putDebugChar(hexchars[checksum % 16]);
  } while (getDebugChar() != '+');            // try until ack ('+') is heard
}

// Routines for access/modify memory
volatile int gdbFaultHandlerSet = 0;  // can activate fault handler for gdb session
volatile int mem_err = 0;             // flags an error from mem2hex/hex2mem

// put mem to buf in hex format and return next buf position
char* mem2hex (const char *mem, char *buf, int count, int may_fault) {
  gdbFaultHandlerSet = (may_fault > 0);
  for (int i = 0; i < count; i++) {
    unsigned char ch = get_char(mem++);
    if (may_fault && mem_err) return buf;
    *buf++ = hexchars[ch >> 4];
    *buf++ = hexchars[ch % 16];
  }
  *buf = 0;
  gdbFaultHandlerSet = 0;

  return buf;
}
// put buf in hex format to mem and return next mem position
char* hex2mem (char *buf, char *mem, int count, int may_fault) {
  gdbFaultHandlerSet = (may_fault > 0);
  for (int i = 0; i < count; i++) {
    unsigned char ch = hex (*buf++) << 4;
    ch = ch + hex (*buf++);
    set_char (mem++, ch);
    if (may_fault && mem_err) return mem;
  }
  gdbFaultHandlerSet = 0;
  return mem;
}

// exception vector to unix signal value
int computeSignal(int64_t exceptionVector) {
  int sigval;
  switch (exceptionVector) {
    case 0: sigval = 8;   break;            // divide by zero
    case 1: sigval = 5;   break;            // debug exception
    case 3: sigval = 5;   break;            // breakpoint
    case 4: sigval = 16;  break;            // into instruction (overflow)
    case 5: sigval = 16;  break;            // bound instruction
    case 6: sigval = 4;   break;            // Invalid opcode
    case 7: sigval = 8;   break;            // coprocessor not available
    case 8: sigval = 7;   break;            // double fault
    case 9: sigval = 11;  break;            // coprocessor segment overrun
    case 10: sigval = 11; break;            // Invalid TSS
    case 11: sigval = 11; break;            // Segment not present
    case 12: sigval = 11; break;            // stack exception
    case 13: sigval = 11; break;            // general protection
    case 14: sigval = 11; break;            // page fault
    case 16: sigval = 7;  break;            // coprocessor error
    default: sigval = 7;                    // "software generated"
  }
  return sigval;
}

// convert hex number in ptr to integer
int hexToInt(char **ptr, long *intValue) {
  int numChars = 0, hexValue = 0;
  *intValue = 0;
  while (**ptr) {
    hexValue = hex(**ptr);
    if (hexValue >= 0) {
      *intValue = (*intValue << 4) | hexValue;
      numChars++;
    } else break;
    ++(*ptr);
  }
  return numChars;
}

// enable/disable stepping
void clearTFBit() {
  reg32 eflags = Gdb::getReg32(registers::EFLAGS);
  eflags &= 0xfffffeff;
  Gdb::setReg32(registers::EFLAGS, eflags);
  DBG::outln(DBG::GDBDebug, "T=", getCurrCpuId(), " TF cleared");
}
void setTFBit() {
  reg32 eflags = Gdb::getReg32(registers::EFLAGS);
  eflags &= 0xfffffeff;
  eflags |= 0x100;
  Gdb::setReg32(registers::EFLAGS, eflags);
  DBG::outln(DBG::GDBDebug, "T=", getCurrCpuId(), " TF set");
}
void clearTFBit(int cpuId) {
  reg32 eflags = Gdb::getReg32(registers::EFLAGS, cpuId);
  eflags &= 0xfffffeff;
  Gdb::setReg32(registers::EFLAGS, eflags, cpuId);
  DBG::outln(DBG::GDBDebug, "T=", cpuId+1, " TF cleared");
}
void setTFBit(int cpuId) {
  reg32 eflags = Gdb::getReg32(registers::EFLAGS, cpuId);
  eflags &= 0xfffffeff;
  eflags |= 0x100;
  Gdb::setReg32(registers::EFLAGS, eflags, cpuId);
  DBG::outln(DBG::GDBDebug, "T=", cpuId+1, " TF set");
}

// vCont packets
// stop reply for vCont packets
void sendStopReply(char* outputBuffer, int sigval) {
  int threadId = getCurrCpuId();
  char* buf = outputBuffer;
  buf[0] = 'T';
  buf[1] = hexchars[sigval >> 4];
  buf[2] = hexchars[sigval % 16];
  buf += 3;
  strcpy(buf, "thread:");
  buf += 7;
  buf[0] = hexchars[threadId >> 4];
  buf[1] = hexchars[threadId % 16];
  buf[2] = ';';
  buf[3] = 0;
  DBG::outln(DBG::GDBDebug, "T=", threadId, " sending stop reply:", outputBuffer);
  putpacket(outputBuffer);
}
// parse and queue vContActions (referenced QEMU's gdbstub)
VContAction* parseVCont(char* ptr) {
  VContAction* result = nullptr;
  int res_signal, res_thread, res = 0;
  long threadId;

  while (*ptr++ == ';') {
    int action; long signal;
    action = *ptr++;
    signal = 0;
    if (action == 'C' || action == 'S') {
      KASSERT0(hexToInt(&ptr, &signal));
    } else if (action != 'c' && action != 's') {
      res = 0;
      break;
    }
    threadId = 0;
    if (*ptr++ == ':') KASSERT0(hexToInt(&ptr, &threadId));
    action = tolower(action);
    if (res == 0 || (res == 'c' && action == 's')) {  // ignore 'c' in vCont;c;s
      res = action;
      res_signal = signal;
      res_thread = threadId;
      DBG::outln(DBG::GDBDebug, "T=", res_thread,
        " action: ", (char)res, " signal: ", res_signal);
    }
  }
  if (res) {    // res is action we decided for current vCont packet
    VContAction* vContAction = new VContAction;
    vContAction->action[0] = res;
    if (res_signal) {
      vContAction->action[1] = hexchars[res_signal >> 4];
      vContAction->action[2] = hexchars[res_signal % 16];
    }
    // thread = -1 means to run vCont packet for all threads (allstop?)
    // thread = 0 means to run vCont packet on any free thread
    if (res_thread != -1 && res_thread != 0) {
      KASSERT1(res_thread <= Gdb::getNumCpusInitialized(), res_thread);
      vContAction->threadId = res_thread;
    }
    vContActionQueue.push(vContAction);
    result = vContAction;
    DBG::outln(DBG::GDBDebug, "T=", getCurrCpuId(), " parsed vCont ", *vContAction);
  }
  return result;
}
// Consumes and executes vContAction for the current thread.
// Precondition: entry_q lock must be held.
void consumeVContAction() {
  VContAction* action = vContActionQueue.front();
  DBG::outln(DBG::GDBDebug, "T=", getCurrCpuId(), "consuming: ", *action);
  KASSERT0(action && !action->executed && action->isForCurrentThread());
  clearTFBit();   // set TF bit if 's' (stepi/step/next)
  if (action->action[0] == 's') setTFBit();
  lockHolder = Processor::getApicID();    // keep entry lock
  if (action->action[0] == 'c') {         // could be from next or continue
    if (breakOnUnwindDebugHook) {         // next
      isBatonFree = true;                 // put down baton to give chance to bp on other threads
      scheduleVContAction();
      _returnFromExceptionLocked();
    } // no return here
    // continue
    isBatonFree = false;                  // keep baton for now and look for thread to pass it to
    // increment rip only if it was actual breakpoint set by user (not set by gdb for next)
    if (prevAction == nullptr || prevAction->action[0] != 'c') Gdb::incrementRip();
    // first, look for a waiting thread to pass baton. If there is no one, then release baton
    for (int i = 0; i < Gdb::getNumCpusInitialized(); i++) {
      if (waiting[i]) {
        waiting[i] = false;
        Gdb::V(i);
        DBG::outln(DBG::GDBDebug, "T=", getCurrCpuId(), " passing baton to ", i+1);
        scheduleVContAction(true);
        _returnFromException();           // do not let other thread race to grab baton before this
      } // no return here
    }
    isBatonFree = true;                   // put down baton
    lockHolder = -1;
    scheduleVContAction(true);
    _returnFromExceptionLocked();
  } // no return here

  // step/next
  scheduleVContAction();
  _returnFromExceptionLocked();
} // no return here
// handles basic 'vCont' commands.
void handleVContPacket(char* ptr, int64_t exceptionVector) {
  if (strncmp(ptr, "Cont", strlen("Cont")) == 0) {
    ptr += 4;
    if (*ptr == '?') strcpy(outputBuffer, "vCont;c;C;s;S");   // supported vCont packets
    else {
      VContAction* action = parseVCont(ptr);
      if (!action) return;                          // must not fail
      entry_q.P();
      if (!action->isForCurrentThread()) {          // wake target thread for vCont packet
        if (!waiting[action->threadId-1]) return;   // must not fail
        DBG::outln(DBG::GDBDebug, "T=", getCurrCpuId(), " passing baton to ", action->threadId);
        Gdb::V(action->threadId-1);                 // pass baton
        handle_exception(exceptionVector);          // fully give control to target thread
      } else {
        isBatonFree = false;                        // make sure I am the only thread talking
        consumeVContAction();
      }
    }
  }
}

// query packet handlers
// gdb front-end queries gdb stub's supported features
static bool handleQSupported(char* in, char* out) {
  if (!strncmp(in, "Supported", 9)) {
    strcpy(out, "PacketSize=1000");       // allow max 4096 bytes per packet
    return true;
  }
  return false;
}
// return current thread ID
static bool handleQC(char* in, char* out) {
  if (*in == 'C') {
    int threadId = Processor::getApicID()+1;
    out[0] = 'Q'; out[1] = 'C';
    out[2] = hexchars[threadId >> 4];
    out[3] = hexchars[threadId % 16];
    out[4] = 0;
    return true;
  }
  return false;
}
// tells if gdb stub process is attached to existing processor created new
static bool handleQAttached(char* in, char* out) {
  if (!strncmp(in, "Attached", 8)) {
    out[0] = '0';   // stub should be killed when session is ended with 'quit'
    out[1] = 0;
    return true;
  }
  return false;
}
// tell section offsets target used when relocating kernel image?
static bool handleQOffsets(char* in, char* out) {
  if (!strncmp(in, "Offsets", 7)) {
    strcpy(out, "Text=0;Data=0;Bss=0");   // FIXME: not 100% sure
    return true;
  }
  return false;
}
// gdb stub can ask for symbol lookups
static bool handleQSymbol(char* in, char* out) {
  if (!strncmp(in, "Symbol::", 8)) {
    strcpy(out, "qSymbol:"); out += 8;
    const char* symbolName = "_Unwind_DebugHook";
    mem2hex(symbolName, out, strlen(symbolName), 0);
    return true;
  }
  return false;
}
// parse replies from above symbol lookup requests (Only looks up _Unwind_DebugHook for now)
static bool handleQSymbolLookup(char* in, char* out) {
  if (!strncmp(in, "Symbol:", 7)) {
    in = in + 7;
    long addr;
    if (hexToInt(&in, &addr)) {
      char buf[50];
      hex2mem(++in, buf, strlen("_Unwind_DebugHook"), 0);
      KASSERT0(!strncmp(buf, "_Unwind_DebugHook", strlen("_Unwind_DebugHook")));
      DBG::outln(DBG::GDBDebug, "lookup address ", FmtHex(addr), '/', buf);
      Gdb::setUnwindDebugHookAddr((mword)addr);
      strcpy(out, "OK");
      return true;
    }
  }
  return false;
}
// query if trace experiment running now
static bool handleQTStatus(char* in, char* out) {
  if (!strncmp(in, "TStatus", 7)) {
    out[0] = 0;     // QEMU sends empty reply
    return true;
  }
  return false;
}
// query list of all active thread IDs
static bool handleQfThreadInfo(char* in, char* out) {
  if (!strncmp(in, "fThreadInfo", 11)) {
    int numCpus = Gdb::getNumCpus();
    out[0] = 'm'; ++out;
    for (int i = 0; i < numCpus; ++i) {
      if (i > 0) {
        out[0] = ','; ++out;
      }
      out[0] = hexchars[(i+1) >> 4];
      out[1] = hexchars[(i+1) % 16];
      out += 2;
    }
    return true;
  }
  return false;
}
// continued querying list of all active thread IDs
static bool handleQsThreadInfo(char* in, char* out) {
  if (!strncmp(in, "sThreadInfo", 11)) {
    out[0] = 'l';
    return true;
  }
  return false;
}
// ask printable string description of a thread's attribute (used by 'info threads')
static bool handleQThreadExtraInfo(char* in, char* out) {
  if (!strncmp(in, "ThreadExtraInfo", 15)) {
    in += 16;
    long threadId;
    if (hexToInt(&in, &threadId)) {
      const char* info = Gdb::getCpuName(threadId-1);
      mem2hex(info, in, strlen(info), 0);
      strcpy(out, in);
      return true;
    }
  }
  return false;
}

// other handlers
// thread alive status
static bool handleT(char* in, char* out) {
  long threadId;
  if (hexToInt(&in, &threadId)) {
    if (threadId >= 1 && threadId <= Gdb::getNumCpus()) {
      strcpy(out, "OK");
      return true;
    }
  }
  strcpy(out, "E05");   // unfortunately not well-defined
  return false;
}
// set which thread will run subsequent operations ('m','M','g','G',etc)
// for step/continue ops ('c'), vCont should be used, instead
static bool handleH(char* in, char* out) {
  char op = *in++;
  long threadId;
  if (hexToInt(&in, &threadId)) {
    if (threadId > -1) {    // threadId == -1 means all threads
      // if threadId == 0, any thread can be used
      if (threadId > 0 && op == 'g') setGCpu(threadId-1);
      strcpy(out, "OK");
      return true;
    }
  }
  strcpy(out, "E05");   // unfortunately not well-defined
  return false;
}
// tells reason target halted (specifically, which thread stopped with what signal)
static bool handleReasonTargetHalted(char* in, char* out, int sigval) {
  int threadId = Processor::getApicID() + 1;
  out[0] = 'T';
  out[1] = hexchars[sigval >> 4];
  out[2] = hexchars[sigval % 16];     out += 3;
  strcpy(out, "thread:");             out += 7;
  out[0] = hexchars[threadId >> 4];
  out[1] = hexchars[threadId % 16];
  out[2] = ';';
  return true;
}
// read general registers
static bool handleReadRegisters(char* in, char* out) {
  GdbCpu* gCpu = getGCpu();
  out = mem2hex((char*)gCpu->getRegs64(), out, numRegs64 * sizeof(reg64), 0);
  out = mem2hex((char*)gCpu->getRegs32(), out, numRegs32 * sizeof(reg32), 0);
  return true;
}
// write general registers
static bool handleWriteRegisters(char* in, char* out) {
  GdbCpu* gCpu = getGCpu();
  hex2mem(in, (char*)gCpu->getRegs64(), numRegs64 * sizeof(reg64), 0);
  hex2mem(in+numRegs64, (char*)gCpu->getRegs32(), numRegs32 * sizeof(reg32), 0);
  strcpy(out, "OK");
  return true;
}
// write register n with value r
// TODO check if register in thread set from 'H' packet is used for current thread
static bool handleWriteRegister(char* in, char* out) {
  long regno;
  GdbCpu* gCpu = getGCpu();
  bool success = false;
  if (hexToInt(&in, &regno) && *in++ == '=') {
    if (regno >= 0 && regno < numRegs64) {
      hex2mem(in, (char *)gCpu->getReg64(regno), sizeof(reg64), 0);
      success = true;
    } else if (regno >= numRegs64 && regno < numRegs) {
      hex2mem(in, (char *)gCpu->getReg32(regno-numRegs64), sizeof(reg32), 0);
      success = true;
    }
  }
  if (success) strcpy(out, "OK");
  else strcpy(out, "E05");        // not well-defined
  return success;
}
// read memory from given address
static bool handleReadMemory(char* in, char* out) {
  long addr, length;
  if (hexToInt(&in, &addr) && *in++ == ',' && hexToInt(&in, &length)) {
    mem_err = 0;
    mem2hex((char *)addr, out, length, 1);
    if (mem_err) strcpy(out, "E03");
    return !mem_err;
  }
  strcpy(out, "E01");
  return false;
}
// write to memory at given address
static bool handleWriteMemory(char* in, char* out) {
  long addr, length;
  if (hexToInt(&in, &addr) && *in++ == ',' && hexToInt(&in, &length) && *in++ == ':') {
    mem_err = 0;
    hex2mem(in, (char *)addr, length, 1);
    if (mem_err) strcpy(out, "E03");
    else strcpy(out, "OK");
    return !mem_err;
  }
  strcpy(out, "E02");
  return false;
}
// set software breakpoints
static bool handleSetSoftBreak(char* in, char* out) {
  long addr, length;
  gdbFaultHandlerSet = 1;             // enable fault handler
  if (hexToInt(&in, &addr) && *in++ == ',' && hexToInt(&in, &length)) {
    BreakPoint* bp = new BreakPoint;
    mem_err = 0;
    char opCode = get_char((char *)addr);
    if (mem_err) goto softbreak_err;
    bp->set(addr, opCode);
    bpList.insertEnd(*bp, true);      // reuse if duplicate exists
    KASSERT1(length == 1, length);    // 1 byte for x86-64?
    DBG::outln(DBG::GDBDebug, "set breakpoint at ", FmtHex(addr));
    MemoryBarrier();                  // unfortunately need this to protect above debug print
    mem_err = 0;
    set_char((char *)addr, 0xcc);     // set break
    if (mem_err) goto softbreak_err;
    if ((mword)addr == Gdb::getUnwindDebugHookAddr()) {
      breakOnUnwindDebugHook = true;  // only set for step/next
      DBG::outln(DBG::GDBDebug, "_Unwind_DebugHook's break set");
    }
    gdbFaultHandlerSet = 0;
    strcpy(out, "OK");
    return true;
  }
softbreak_err:
  gdbFaultHandlerSet = 0;
  strcpy(out, "E03");     // not well-defined
  return false;
}
// remove software breakpoints
static bool handleRemoveSoftBreak(char* in, char* out) {
  long addr, length;
  gdbFaultHandlerSet = 1;             // enable fault handler
  if (hexToInt(&in, &addr) && *in++ == ',' && hexToInt(&in, &length)) {
    BreakPoint curBp = { (mword)addr, *(char *)addr };
    Node<BreakPoint>* bp = bpList.search(curBp);
    if (bp) {
      mem_err = 0;
      set_char((char *)addr, bp->getElement().getOpcode());
      if (mem_err) goto softbreak_err;
      MemoryBarrier();
      DBG::outln(DBG::GDBDebug, "remove breakpoint at ", FmtHex(addr));
      if ((mword)addr == Gdb::getUnwindDebugHookAddr()) {
        breakOnUnwindDebugHook = false;   // remove always as a last step before step/next returns
        DBG::outln(DBG::GDBDebug, "_Unwind_DebugHook's break removed");
      }
      gdbFaultHandlerSet = 0;
      strcpy(out, "OK");
      return true;
    }
  }
softbreak_err:
  gdbFaultHandlerSet = 0;
  strcpy(out, "E03");       // not well-defined
  return false;
}
// kill request (how to kill?)
static bool handleKill(char* in, char* out) {
  ABORT0();   // No description on what to do when individual thread is selected
  return true;
}

// does all command processing for interfacing to gdb
extern "C" void handle_exception (int64_t exceptionVector) {
  int sigval = computeSignal(exceptionVector);
  int threadId = Processor::getApicID();
  char* ptr = outputBuffer;

  KASSERT0(!Processor::interruptsEnabled());
  enablePreemption();     // restore preemption if disabled by gdb stub
  Gdb::resetRip();        // restore rip manipulation
  Gdb::setCpuState(cpuState::BREAKPOINT);

  DBG::outln(DBG::GDBDebug, "T=", getCurrCpuId(), ", vector=", exceptionVector,
            ", eflags=", FmtHex(Gdb::getReg32(registers::EFLAGS)),
            ", pc=", FmtHex(Gdb::getReg64(registers::RIP)));

  // for breakpoints that gdb frontend sets, it inserts 0xCC to first byte of an instruction
  // instead of setting TF bit. gdb frontend does not replace the original byte. Therefore,
  // rip we get is one byte more than it should be because 0xCC (int3) is one instruction.
  if (exceptionVector == 3) {
    reg32 eflags = Gdb::getReg32(registers::EFLAGS);
    if ((eflags & 0x100) == 0) {    // gdb frontend inserted breakpoint
      Gdb::decrementRip();
      DBG::outln(DBG::GDBDebug, "rip decremented to ", FmtHex(Gdb::getReg64(registers::RIP)));
    }
  }

  // baton grabbing procedure
  DBG::outln(DBG::GDBDebug, "T=", getCurrCpuId(), " trying to grab a baton");
  entry_q.P();    // entry protocol
  if (!canGrabBaton(threadId)) {
    waiting[threadId] = true;       // failed to grab a baton; wait until someone gives it to me
    DBG::outln(DBG::GDBDebug, "T=", getCurrCpuId(), " will wait for ", lockHolder+1);
    entry_q.V();  // entry protocol end
    Gdb::P(threadId);               // wait till the baton is passed
    DBG::outln(DBG::GDBDebug, "T=", getCurrCpuId(), " holds a baton");
    if (shouldReturnFromException[threadId]) {    // for allstop mode
      DBG::outln(DBG::GDBDebug, "T=", getCurrCpuId(), " exiting");
      shouldReturnFromException[threadId] = false;
      Gdb::setCpuState(cpuState::RUNNING);
      restoreRegisters(Gdb::getCurrentCpu());    // exit to KOS
    }
  }

  // you are a new baton owner!
  isBatonFree = false;                  // baton is not available to others
  waiting[threadId] = false;            // I'm not waiting for a baton anymore
  lockHolder = threadId;                // I'm the baton holder!
  memset(outputBuffer, 0, BUFMAX);      // reset output buffer
  DBG::outln(DBG::GDBDebug, "T=", getCurrCpuId(), " holds a baton");

  // send stop reply for executed vCont packet or consume a pending vCont packet
  if (!vContActionQueue.empty()) {
    VContAction* action = vContActionQueue.front();
    if (action->executed) {             // did I just execute vCont packet?
      DBG::outln(DBG::GDBDebug, "T=", getCurrCpuId(), " done:", *action);
      prevAction = action;              // remember previous action (if 'c' don't increment rip)
      vContActionQueue.pop();
      KASSERT0(vContActionQueue.empty());
      sendStopReply(outputBuffer, sigval);
    } else {                            // someone must have given me a baton to execute this
      DBG::outln(DBG::GDBDebug, "T=", getCurrCpuId(), " consume:", *action);
      consumeVContAction();
    }
  }
  if (allstop) Gdb::sendIPIToAllOtherCores(0x1);    // break all other threads for allstop mode
  entry_q.V();                  // entry protocol exit

  while (true) {
    memset(outputBuffer, 0, BUFMAX);
    ptr = getpacket();          // get request from gdb
    switch (*ptr++) {
      case 'q': {   // queries
        if (handleQSupported(ptr, outputBuffer)) break;
        else if (handleQC(ptr, outputBuffer)) break;
        else if (handleQAttached(ptr, outputBuffer)) break;
        else if (handleQOffsets(ptr, outputBuffer)) break;
        else if (handleQSymbol(ptr, outputBuffer)) break;
        else if (handleQSymbolLookup(ptr, outputBuffer)) break;
        else if (handleQTStatus(ptr, outputBuffer)) break;
        else if (handleQfThreadInfo(ptr, outputBuffer)) break;
        else if (handleQsThreadInfo(ptr, outputBuffer)) break;
        else if (handleQThreadExtraInfo(ptr, outputBuffer)) break;
        // TODO if in any case you encounter this, time to support new packets
        // strcpy(outputBuffer, "qUnimplemented");
      } break;
      case 'v': handleVContPacket(ptr, exceptionVector); break; // vCont packets
      case 'T': handleT(ptr, outputBuffer); break;              // is thread alive?
      case 'H': handleH(ptr, outputBuffer); break;    // set thread for subsequent operation
      case '?': handleReasonTargetHalted(ptr, outputBuffer, sigval); break;
      case 'g': handleReadRegisters(ptr, outputBuffer); break;  // read general registers
      case 'G': handleWriteRegisters(ptr, outputBuffer); break; // write general registers
      case 'P': handleWriteRegister(ptr, outputBuffer); break;  // write a register
      case 'm': handleReadMemory(ptr, outputBuffer); break;     // read memory
      case 'M': handleWriteMemory(ptr, outputBuffer); break;    // write to memory
      case 'Z': {   // set breaks, watchpoints, etc
        long int type;
        if (hexToInt(&ptr, &type)) {
          ptr++;
          switch (type) {
            case 0: handleSetSoftBreak(ptr, outputBuffer); break;     // set soft breakpoint
          }
        }
      } break;
      case 'z': {
        long int type;
        if (hexToInt(&ptr, &type)) {
          ptr++;
          switch (type) {
            case 0: handleRemoveSoftBreak(ptr, outputBuffer); break;  // remove soft breakpoint
          }
        }
      } break;
      case 'k': handleKill(ptr, outputBuffer); break;           // kill KOS
    } // switch

    // reply to the request
    putpacket(outputBuffer);
  }
}

// hijack KOS ISR's to gdb stub's version
void set_debug_traps(bool as) {
  exceptionHandler (0x00, catchException0x00);
  exceptionHandler (0x01, catchException0x01);
  exceptionHandler (0x03, catchException0x03);
  exceptionHandler (0x04, catchException0x04);
  exceptionHandler (0x05, catchException0x05);
  exceptionHandler (0x06, catchException0x06);
  exceptionHandler (0x07, catchException0x07);
  exceptionHandler (0x0c, catchExceptionFault0x0c);
  exceptionHandler (0x0d, catchExceptionFault0x0d);
  exceptionHandler (0x0e, catchExceptionFault0x0e);
  exceptionHandler (0x10, catchException0x10);

  waiting = new bool[Gdb::getNumCpus()];
  shouldReturnFromException = new bool[Gdb::getNumCpus()];
  gCpu = new GdbCpu*[Gdb::getNumCpus()];
  preempt = new bool[Gdb::getNumCpus()];
  for (int i = 0; i < Gdb::getNumCpus(); i++) {
    waiting[i] = false;
    shouldReturnFromException[i] = false;
    gCpu[i] = nullptr;
    preempt[i] = true;
  }
  entry_q.resetCounter(1);  // simulate mutex

  allstop = as;
  initialized = 1;
}

// call this after set_debug_traps to initiate gdb session
void breakpoint() {
  if (initialized) BREAKPOINT();
}
