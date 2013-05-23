#include <cstring>
#include <cstdarg>
#include <cstdint>

#include "gdb/GdbCpu.h"
#include "gdb/gdb.h"
#include "gdb/gdb_asm_functions.h"
#include "gdb/SpinLock.h"
#include "kern/Thread.h"
#include "mach/Processor.h"
#include "util/Debug.h"
#include "util/SpinLock.h"
#include "util/EmbeddedQueue.h"

#undef __STRICT_ANSI__
#include <cstdio>

using namespace gdb;
using namespace std;

// function stubs for GDB remote serial protocol
extern void putDebugChar(unsigned char ch);         // write a single character
extern int getDebugChar();                          // read and return a single char
extern void exceptionHandler(int, void (*)());      // assign an exception handler

extern "C" void handle_exception(int64_t);

static const int BUFMAX = 4096;     // max number of chars in inbound/outbound buffers
static char initialized;            // boolean flag. != 0 means we've been initialized
static int remote_debug;            // debug > 0 prints ill-formed commands
                                    // in valid packets & checksum errors

static const char hexchars[]="0123456789abcdef";

#define BREAKPOINT() asm("int $3");

static bool isFree = true;          // baton is not picked up.

static VContAction* isContinue = NULL; // VContAction for 'c' from previous thread.
                                    // it is used to notify the current thread
                                    // to send a stop reply packet for previous thread.

struct ThreadSeen {
  Thread* currThread;               // current thread
  Thread* prevThread;               // last seen thread

  ThreadSeen(): currThread(0), prevThread(0) {}
};

struct BSPThreadInfo {
  Thread* thread;
  int cpuIdx;

  BSPThreadInfo(): thread(0), cpuIdx(-1) {}
};

int lockHolder = -1;                // used for holding baton.
bool* waiting;                      // represents a CPU waiting on entry queue.
bool* shouldReturnFromException;
int* coreIdxTable;                  // maps current core index
ThreadSeen* threadsSeen;            // queues all threads seen for each core.
BSPThreadInfo bspThreadInfo;
//bool halted = false;                // represent if the source rip is hlt.
//bool paused = false;                // represent if the source ip is pause.
gdb::GdbSpinLock entry_q;           // entry queue mutex. (part of split binary semaphore)
//SpinLock entry_q;

//DBG::Level debugLevel = (DBG::test(DBG::AllStopGDB) ? DBG::AllStopGDB : DBG::GDB);

int getCurrentCoreIdx() {
  return coreIdxTable[Processor::getApicID()];
}

static bool allstop = false;

// Notify next thread to send stop reply.
void setContinue(VContAction* cont) {
  KASSERT0(!isContinue);
  VContAction* action = GDB::getInstance().nextVContAction(getCurrentCoreIdx());
  KASSERT1(*cont == *action, "Cannot set continue action if it's not first vCont in queue");
  isContinue = cont;
  GDB::getInstance().popVContAction(getCurrentCoreIdx());
}

void clearContinue() {
  KASSERT0(isContinue);
  isContinue = NULL;
}

/**
 * Returns from the exception handler.
 * If source rip was a halt instruction, update cpu state to halted.
 * Otherwise, update cpu state to running. (freely executing)
 *
 * Also, for all-stop mode, all waiting threads should return.
 */
void _returnFromException(int cpuState = 0) {
  GdbCpuState* state = GDB::getInstance().getCurrentCpuState();
  switch (cpuState) {
    case 0: state->setCpuState(cpuState::RUNNING); break;
    case 1: state->setCpuState(cpuState::HALTED); break;
    case 2: state->setCpuState(cpuState::PAUSED); break;
  }

  // continue all other threads in all-stop mode.
  if (allstop) {
    int threadId = Processor::getApicID();
    for (int i = 0; i < GDB::getInstance().getNumCpusInitialized(); i++) {
      if (coreIdxTable[i] == threadId) continue;
      if (waiting[coreIdxTable[i]]) {
        waiting[coreIdxTable[i]] = false;                   // wake thread and force it to return
        shouldReturnFromException[coreIdxTable[i]] = true;
        GDB::getInstance().V(i);
      }
    }
  }
//  entry_q.releaseISR();
  state->restoreRegisters();
}

// use if entry_q is not locked
void _returnFromExceptionLocked(int cpuState = 0) {
  entry_q.acquireISR();
  GdbCpuState* state = GDB::getInstance().getCurrentCpuState();
  switch (cpuState) {
    case 0: state->setCpuState(cpuState::RUNNING); break;
    case 1: state->setCpuState(cpuState::HALTED); break;
    case 2: state->setCpuState(cpuState::PAUSED); break;
  }

  // continue all other threads in all-stop mode.
  if (allstop) {
    int threadId = Processor::getApicID();
    for (int i = 0; i < GDB::getInstance().getNumCpusInitialized(); i++) {
      if (coreIdxTable[i] == threadId) continue;
      if (waiting[coreIdxTable[i]]) {
        waiting[coreIdxTable[i]] = false;                   // wake the thread and force it to return
        shouldReturnFromException[coreIdxTable[i]] = true;
        GDB::getInstance().V(i);
      }
    }
  }
  entry_q.releaseISR();
  state->restoreRegisters();
}

int hex (char ch) {
  if ((ch >= 'a') && (ch <= 'f')) return (ch - 'a' + 10);
  if ((ch >= '0') && (ch <= '9')) return (ch - '0');
  if ((ch >= 'A') && (ch <= 'F')) return (ch - 'A' + 10);
  return (-1);
}

static char inputBuffer[BUFMAX];             // input buffer
static char outputBuffer[BUFMAX];            // output buffer

// scan for the sequence $<data>#<checksum>
char* getpacket () {
  char *buffer = &inputBuffer[0];
  char checksum;
  char xmitcsum;
  int count;
  char ch;

  while (1) {
    /* wait around for the start character, ignore all other characters */
    while ((ch = getDebugChar ()) != '$')
    ;

  retry:
    checksum = 0;
    xmitcsum = -1;
    count = 0;

    /* now, read until a # or end of buffer is found */
    while (count < BUFMAX - 1) {
      ch = getDebugChar ();
      if (ch == '$') goto retry;
      if (ch == '#') break;
      checksum = checksum + ch;
      buffer[count] = ch;
      count = count + 1;
    }
    buffer[count] = 0;

    if (ch == '#') {
      ch = getDebugChar ();
      xmitcsum = hex (ch) << 4;
      ch = getDebugChar ();
      xmitcsum += hex (ch);

      if (checksum != xmitcsum) {
        if (remote_debug) {
          fprintf (stderr,
            "bad checksum.  My count = 0x%x, sent=0x%x. buf=%s\n",
            checksum, xmitcsum, buffer);
        }
        putDebugChar ('-');   /* failed checksum */
      } else {
        putDebugChar ('+');   /* successful transfer */
        /* if a sequence char is present, reply the sequence ID */
        if (buffer[2] == ':') {
          putDebugChar(buffer[0]);
          putDebugChar(buffer[1]);
          return &buffer[3];
        }

        return &buffer[0];
      }
    }
  }
}

// send the packet in buffer
void putpacket (char *buffer) {
  unsigned char checksum;
  int count;
  char ch;

  //  $<packet info>#<checksum>.
  do {
    putDebugChar ('$');
    checksum = 0;
    count = 0;

    while ( (ch = buffer[count]) ) {
      putDebugChar(ch);
      checksum += ch;
      count += 1;
    }

    putDebugChar('#');
    putDebugChar(hexchars[checksum >> 4]);
    putDebugChar(hexchars[checksum % 16]);
  } while (getDebugChar () != '+');
}

void debug_error (const char *format, ...) {
  if (remote_debug) {
    va_list parm;
    va_start(parm, format);
    fprintf (stderr, format, parm);
    va_end(parm);
  }
}

// Address of a routine to RTE to if we get a memory fault.
void (*volatile mem_fault_routine) () = NULL;

// Indicate to caller of mem2hex or hex2mem that there has been an error
static volatile int mem_err = 0;

void set_mem_err () {
  mem_err = 1;
}

// These are separate functions so that they are so short and sweet
// that the compiler won't save any registers (if there is a fault
// to mem_fault, they won't get restored, so there better not be any
// saved)
int get_char (char *addr) {
  return *addr;
}

void set_char (char *addr, int val) {
  *addr = val;
}

// convert the memory pointed to by mem into hex, placing result in buf
// return a pointer to the last char put in buf (null)
// If MAY_FAULT is non-zero, then we should set mem_err in response to
// a fault; if zero treat a fault like any other fault in the stub
char* mem2hex (char *mem, char *buf, int count, int may_fault) {
  // HACK
  if (!mem) {
    for (int i = 0; i < count; i++) {
      *buf++ = '0';
      *buf++ = '0';
    }
    *buf = 0;
    return buf;
  }

  if (may_fault) mem_fault_routine = set_mem_err;
  for (int i = 0; i < count; i++) {
    unsigned char ch = get_char(mem++);
    if (may_fault && mem_err) return buf;
    *buf++ = hexchars[ch >> 4];
    *buf++ = hexchars[ch % 16];
  }
  *buf = 0;
  if (may_fault) mem_fault_routine = NULL;

  return buf;
}

// convert the hex array pointed to by buf into binary to be placed in mem
// return a pointer to the character AFTER the last byte written
char* hex2mem (char *buf, char *mem, int count, int may_fault) {
  if (may_fault) mem_fault_routine = set_mem_err;
  for (int i = 0; i < count; i++) {
    unsigned char ch = hex (*buf++) << 4;
    ch = ch + hex (*buf++);
    set_char (mem++, ch);
    if (may_fault && mem_err) return mem;
  }
  if (may_fault) mem_fault_routine = NULL;
  return mem;
}

// this function takes the x86_64 exception vector and attempts to
// translate this number into a unix compatible signal value
int computeSignal (int64_t exceptionVector) {
  int sigval;
  switch (exceptionVector) {
    case 0: sigval = 8;   break;            /* divide by zero */
    case 1: sigval = 5;   break;            /* debug exception */
    case 3: sigval = 5;   break;            /* breakpoint */
    case 4: sigval = 16;  break;            /* into instruction (overflow) */
    case 5: sigval = 16;  break;            /* bound instruction */
    case 6: sigval = 4;   break;            /* Invalid opcode */
    case 7: sigval = 8;   break;            /* coprocessor not available */
    case 8: sigval = 7;   break;            /* double fault */
    case 9: sigval = 11;  break;            /* coprocessor segment overrun */
    case 10: sigval = 11; break;            /* Invalid TSS */
    case 11: sigval = 11; break;            /* Segment not present */
    case 12: sigval = 11; break;            /* stack exception */
    case 13: sigval = 11; break;            /* general protection */
    case 14: sigval = 11; break;            /* page fault */
    case 16: sigval = 7;  break;            /* coprocessor error */
    default: sigval = 7;                    /* "software generated" */
  }
  return sigval;
}

// While we find nice hex chars, build an int
// return number of chars processed
int hexToInt (char **ptr, long *intValue) {
  int numChars = 0;
  int hexValue;

  *intValue = 0;
  while (**ptr) {
    hexValue = hex (**ptr);
    if (hexValue >= 0) {
      *intValue = (*intValue << 4) | hexValue;
      numChars++;
    } else break;

    (*ptr)++;
  }

  return numChars;
}

/**
 * Detects if the instruction causing interrupt
 * is halt and update CPU state and pass baton
 * to waiting thread or release baton.
 */
bool checkIfHalt(reg64 rip) {
  char* str = (char *) rip;
  char hlt[2];    // hlt's opcode is f4
  hlt[0] = hexchars[(str[0] >> 4) & 0xf];
  hlt[1] = hexchars[(str[0] % 16) & 0xf];
  if (hlt[0] == 'f' && hlt[1] == '4') {
    DBG::outlnISR(DBG::GDBDebug, "halt at thread ", Processor::getApicID() + 1);
    return true;
  }
  return false;
}

/**
 * Detects if the instruction causing interrupt
 * is pause and update CPU state and pass baton
 * to waiting thread or release it.
 */
bool checkIfPause(reg64 rip) {
  char* str = (char *) rip;
  char pause[4];  // pause's opcode is f3 90
  pause[0] = hexchars[(str[0] >> 4) & 0xf];
  pause[1] = hexchars[(str[0] % 16) & 0xf];
  pause[2] = hexchars[(str[1] >> 4) & 0xf];
  pause[3] = hexchars[(str[1] % 16) & 0xf];
  if (pause[0] == 'f' && pause[1] == '3' &&
    pause[2] == '9' && pause[3] == '0') {
    DBG::outlnISR(DBG::GDBDebug, "pause at thread ", Processor::getApicID() + 1);
    return true;
  }
  return false;
}

/**
 * Clears TF bit from current thread's eflags.
 * TF should be cleared before returning from handler
 * if not stepping.
 */
void clearTFBit() {
  GdbCpuState* state = GDB::getInstance().getCurrentCpuState();
  reg32 val = *(state->getReg32(registers::EFLAGS));
  val &= 0xfffffeff;
  state->setReg32(registers::EFLAGS, val);

  DBG::outlnISR(DBG::GDBDebug, "Cleared TF bit for thread ",
    Processor::getApicID() + 1, " eflags: ", FmtHex(val));
}

/**
 * Sets TF bit from current thread's eflags.
 * TF should be set before returning from handler
 * for stepping.
 */
void setTFBit() {
  GdbCpuState* state = GDB::getInstance().getCurrentCpuState();
  reg32 val = *(state->getReg32(registers::EFLAGS));
  val &= 0xfffffeff;
  val |= 0x100;
  state->setReg32(registers::EFLAGS, val);

  DBG::outlnISR(DBG::GDBDebug, "Set TF bit for thread ",
    Processor::getApicID() + 1, " eflags: ", FmtHex(val));
}

/**
 * Generates stop reply for vCont packets.
 */
void sendStopReply(char* outputBuffer, int sigval) {
  int threadId = Processor::getApicID() + 1;
  outputBuffer[0] = 'T';
  outputBuffer[1] = hexchars[sigval >> 4];
  outputBuffer[2] = hexchars[sigval % 16];
  outputBuffer[3] = 't';
  outputBuffer[4] = 'h';
  outputBuffer[5] = 'r';
  outputBuffer[6] = 'e';
  outputBuffer[7] = 'a';
  outputBuffer[8] = 'd';
  outputBuffer[9] = ':';
  outputBuffer[10] = hexchars[threadId >> 4];
  outputBuffer[11] = hexchars[threadId % 16];
  outputBuffer[12] = ';';
  outputBuffer[13] = 0;

  DBG::outlnISR(DBG::GDBDebug, "Sending stop reply: ", outputBuffer,
    " from thread ", Processor::getApicID() + 1);
  putpacket(outputBuffer);
}

/**
 * Parse and queue vContActions.
 * From QEMU's gdbstub, I decided to assume only 1 action
 * is given from each vCont packet.
 */
VContAction* parse_vcont(char* ptr)
{
  VContAction* result = NULL;

  int res_signal, res_thread, res = 0;
  long threadId;

  while (*ptr == ';') {
    int action; long signal;
    ptr++;
    action = *ptr++;
    signal = 0;
    if (action == 'C' || action == 'S') {
      int read = hexToInt(&ptr, &signal);
      KASSERT0(read);
    }
    threadId = 0;
    if (*ptr++ == ':') {
      int read = hexToInt(&ptr, &threadId);
      KASSERT0(read);
    }
    if (action == 'C') action = 'c';
    else if (action == 'S') action = 's';
    if (res == 0 || (res == 'c' && action == 's')) {
      res = action;
      res_signal = signal;
      res_thread = threadId;
      DBG::outlnISR(DBG::GDBDebug, "action: ", (char)res, " signal: ", res_signal,
        " thread: ", res_thread);
    }
  }

  if (res) {
    VContAction* vContAction = new VContAction;
    vContAction->action[0] = res;
    if (res_signal) {
      vContAction->action[1] = hexchars[res_signal >> 4];
      vContAction->action[2] = hexchars[res_signal % 16];
    }
    vContAction->empty = false;
    if (res_thread != -1 && res_thread != 0) {
      KASSERT1(res_thread <= GDB::getInstance().getNumCpusInitialized(), res_thread);
      vContAction->threadId = res_thread;
      vContAction->threadAssigned = true;
      GDB::getInstance().addVContAction(vContAction, coreIdxTable[res_thread-1]);
      DBG::outlnISR(DBG::GDBDebug, "putting vContAction to ", coreIdxTable[res_thread-1]+1);
//      GDB::getInstance().addVContAction(vContAction, res_thread-1);
    } else {
      // current thread will handle action if thread = -1 or 0
      GDB::getInstance().addVContAction(vContAction, getCurrentCoreIdx());
      DBG::outlnISR(DBG::GDBDebug, "putting vContAction to ", getCurrentCoreIdx()+1);
//      GDB::getInstance().addVContAction(vContAction);
    }

    result = vContAction;
    DBG::outlnISR(DBG::GDBDebug, "Parsed vContAction - ", *vContAction);
  }

  return result;
}

/**
 * Consumes and executes vContAction for the current thread.
 *
 * Precondition: entry_q lock must be held.
 */
void consumeVContAction()
{
  VContAction* action = GDB::getInstance().nextVContAction(getCurrentCoreIdx());
  DBG::outlnISR(DBG::GDBDebug, *action, " from thread: ", Processor::getApicID() + 1);

  KASSERT0(action);
  KASSERT1(!action->executed, action->action);

  uint32_t threadId = (action->threadId == -1 || action->threadId == 0 ?
                          Processor::getApicID() :
                          action->threadId-1);
  KASSERT1(threadId == Processor::getApicID(), threadId);

  clearTFBit();
  if (action->action[0] == 's') {
    setTFBit();
  }

  reg32 eflags = *GDB::getInstance().getCurrentCpuState()->getReg32(registers::EFLAGS);
  DBG::outlnISR(DBG::GDBDebug, "Consuming vContAction - ", *action,
       " from thread: ", Processor::getApicID() + 1,
       " eflags: ", FmtHex(eflags));

  // When lockHolder is set, other threads cannot
  // acquire the entry lock fully if isFree is set to false.
  // (Prevent other threads from entering the handler
  // before the stop reply for the 's' is sent).
  lockHolder = threadId;

  // let other thread to enter interrupt handler
  if (action->action[0] == 'c') {
    // trap bit should have been cleared.
    KASSERT1((eflags & 0x00000100) == 0, FmtHex(eflags));
    isFree = true;
    setContinue(action);  // set isContinue so that subsequent thread will send stop reply.
    GDB::getInstance().getCurrentCpuState()->incrementRip();  // increment rip if rip was decremented
    // if there's a thread waiting on breakpoint, switch to that thread.
    for (int i = 0; i < GDB::getInstance().getNumCpusInitialized(); i++) {
      if (waiting[i]) {
        isFree = false;
        waiting[i] = false;
        GDB::getInstance().V(i);
        DBG::outlnISR(DBG::GDBDebug, "Passed baton from thread: ",
          Processor::getApicID() + 1, " to thread: ", i + 1);
        action->executed = true;
        _returnFromException();
        // does not return here
      }
    }
  }

  // first thread that enters the interrupt handler
  // will send stop reply packet.
  action->executed = true;
  entry_q.releaseISR();
  _returnFromExceptionLocked();
}

/**
 * Handles 'vCont' commands.
 * TODO: support vAttach, vFile, vFlashErase, vFlashWrite,
 * vFlashDone, vKill, vRun, vStopped.
 */
void gdb_cmd_vresume(char* ptr, int64_t exceptionVector)
{
  if (strncmp(ptr, "Cont", strlen("Cont")) == 0) {
    ptr += strlen("Cont");
    if (*ptr == '?') {
      strcpy(outputBuffer, "vCont;c;C;s;S");
    } else {
      // vCont[;action[:thread-id]]...
      // FIXME: currently assumes only one action per vCont
      // (referenced QEMU 1.4.1, gdbstub.c)
      VContAction* action = parse_vcont(ptr);
      KASSERT0(action);
      if (action) {
        entry_q.acquireISR();
        // if the action is not for the current thread,
        // pass the baton to the waiting thread.
        if (action->threadId != -1 && action->threadId != 0 &&
              coreIdxTable[action->threadId-1] != coreIdxTable[(int)Processor::getApicID()]) {
          if (waiting[coreIdxTable[action->threadId-1]]) {
            // pass baton to the waiting thread.
            GDB::getInstance().V(action->threadId-1);
            DBG::outlnISR(DBG::GDBDebug, "Passed baton from thread: ",
              Processor::getApicID() + 1, " to thread: ", action->threadId - 1);
            handle_exception(exceptionVector);  // this thread will wait.
          }

          // Assumes the thread needs to be in
          // interrupt handler before GDB asks 'c', or 's'
          DBG::outlnISR(DBG::GDBDebug, "current thread: ", Processor::getApicID() + 1);
          ABORT1(action->threadId);
        } else {
          // action on current thread.
          isFree = false;         // hold on to the baton
          consumeVContAction();
        }
      }
    }
  }
}

void updateFlagsOnThreadSwitch() {
  int threadId = Processor::getApicID();
  Thread* currThread = Processor::getCurrThread();

  if (bspThreadInfo.thread == nullptr) {
    bspThreadInfo.thread = currThread;
    bspThreadInfo.cpuIdx = threadId;
  } else {
    if (threadId != bspThreadInfo.cpuIdx && currThread == bspThreadInfo.thread) {
      DBG::outlnISR(DBG::GDBDebug, "switching from core ", bspThreadInfo.cpuIdx+1, " to core ", threadId+1);
      coreIdxTable[threadId] = coreIdxTable[bspThreadInfo.cpuIdx];
      coreIdxTable[bspThreadInfo.cpuIdx] = bspThreadInfo.cpuIdx;      // reset cpuIdx for previous core
      if (lockHolder == bspThreadInfo.cpuIdx) lockHolder = threadId;
      bspThreadInfo.thread = currThread;
      bspThreadInfo.cpuIdx = threadId;
    } else if (threadId == bspThreadInfo.cpuIdx && currThread != bspThreadInfo.thread) {
      DBG::outlnISR(DBG::GDBDebug, "thread switched from ", bspThreadInfo.thread, " to ", currThread, " for thread ", threadId+1);
      bspThreadInfo.thread = currThread;
    } else if (threadId != bspThreadInfo.cpuIdx && currThread != bspThreadInfo.thread) {
      bspThreadInfo.thread = currThread;
      bspThreadInfo.cpuIdx = threadId;
    }
  }

  DBG::outlnISR(DBG::GDBDebug, "bspThreadInfo.thread=", bspThreadInfo.thread, " bspThreadInfo.cpuIdx=", bspThreadInfo.cpuIdx+1);
/*
  // only queue a thread if it is different than what's queued
  if (threadsSeen[threadId].currThread != NULL) {
    if (threadsSeen[threadId].currThread == currThread) {
      Logger::debugln("queue front ", FmtHex(threadsSeen[threadId].currThread), " curThread: ", FmtHex(currThread));
      return;
    }
  }
  Logger::debugln("core ", threadId+1, " currThread: ", threadsSeen[threadId].currThread);
  Logger::debugln("core ", threadId+1, " prevThread: ", threadsSeen[threadId].prevThread);
  threadsSeen[threadId].currThread = currThread;
  if (threadsSeen[threadId].prevThread == NULL) threadsSeen[threadId].prevThread = currThread;
  Logger::debugln("push thread ", currThread, " to core ", threadId+1);
  bool found = false;
  for (int i = 0; i < GDB::getInstance().getNumCpusInitialized(); i++) {
    if (threadId == i) continue;
    if (threadsSeen[i].currThread != NULL) {
      Thread* thread = threadsSeen[i].currThread;
      if (thread == currThread) {   // core i switched to core threadId
        Logger::debugln("core ", i+1, " switched to core ", threadId+1);
        coreIdxTable[threadId] = i;
        if (lockHolder == i) lockHolder = threadId;
        threadsSeen[threadId].prevThread = threadsSeen[threadId].currThread;
        threadsSeen[threadId].currThread = threadsSeen[i].currThread;
        threadsSeen[i].prevThread = threadsSeen[i].currThread;
        threadsSeen[i].currThread = NULL;
        found = true;
        break;
      }
    }
  }
*/
}

// does all command processing for interfacing to gdb
extern "C" void
handle_exception (int64_t exceptionVector) {
  int sigval;
  long addr, length;
  char *ptr;

  KASSERT0(!Processor::interruptsEnabled());

  int threadId = Processor::getApicID();
  GDB::getInstance().getCurrentCpuState()->setCpuState(cpuState::BREAKPOINT);

  DBG::outlnISR(DBG::GDBDebug, "thread=", threadId+1, ", vector=", exceptionVector,
            ", eflags=", FmtHex(*GDB::getInstance().getCurrentCpuState()->getReg32(registers::EFLAGS)),
            ", pc=", FmtHex(*GDB::getInstance().getCurrentCpuState()->getReg64(registers::RIP)),
            ", thread=", FmtHex(Processor::getCurrThread()));

  // update gdb's internal flags if the thread has been switched between cores.
  updateFlagsOnThreadSwitch();

  // for int 3, rip pushed by interrupt handling mechanism may not be a correct value
  // decrement rip value back to an instruction causing breakpoint and let it try again with a valid address
  GDB::getInstance().getCurrentCpuState()->resetRip();
  if (exceptionVector == 3) {
    reg32 eflags = *GDB::getInstance().getCurrentCpuState()->getReg32(registers::EFLAGS);
    DBG::outlnISR(DBG::GDBDebug, "eflags & 0x100 = ", (eflags & 0x100));
    if ((eflags & 0x100) == 0) {   // if TF is set, treat the rip valid
      GDB::getInstance().getCurrentCpuState()->decrementRip();
      DBG::outlnISR(DBG::GDBDebug, "thread ", threadId+1, " decrement rip to ",
        FmtHex(*GDB::getInstance().getCurrentCpuState()->getReg64(registers::RIP)));
    }
  }

  sigval = computeSignal(exceptionVector);
  ptr = outputBuffer;

  /**
   * If a baton is held by another thread and the lockholder is not you,
   * you have to wait until someone passes a baton.
   */
  DBG::outlnISR(DBG::GDBDebug, "thread ", threadId+1, " trying to enter");
  entry_q.acquireISR();

  if (!isFree && (lockHolder == -1 || lockHolder != threadId)) {
    waiting[threadId] = true;
    DBG::outlnISR(DBG::GDBDebug, "thread ", threadId+1,
      " is waiting for real lock holder: ", lockHolder+1);
    entry_q.releaseISR();
    GDB::getInstance().P(threadId);
    DBG::outlnISR(DBG::GDBDebug, "thread ", threadId+1, " woke up");
    if (shouldReturnFromException[threadId]) {
      DBG::outlnISR(DBG::GDBDebug, "thread ", threadId+1, " returning from exception");
      shouldReturnFromException[threadId] = false;
      GdbCpuState* state = GDB::getInstance().getCurrentCpuState();
      state->setCpuState(cpuState::RUNNING);
      state->restoreRegisters();
    }
  }

  // Got hold of a baton. You are free to enter.
  isFree = false;
  waiting[coreIdxTable[threadId]] = false;
  lockHolder = threadId;
  DBG::outlnISR(DBG::GDBDebug, "thread ", threadId+1, " got in");

  memset(outputBuffer, 0, BUFMAX);      // reset output buffer

  /**
   * Previous thread executed 'c' (cotinue)
   * Follow-up thread must send stop reply message
   * to inform GDB that it is talking to the new thread.
   */
  if (isContinue) {
    sendStopReply(outputBuffer, sigval);
    clearContinue();

    // TODO handle pending VContAction later??
    goto begin;
  }

  DBG::outlnISR(DBG::GDBDebug, "thread ", threadId+1, " is mapped to thread ", getCurrentCoreIdx()+1);
  if (GDB::getInstance().isEmptyVContActionQueue(getCurrentCoreIdx())) {
    DBG::outlnISR(DBG::GDBDebug, "vContAction queue is empty for thread ", getCurrentCoreIdx()+1);
  }

  // Execute a pending operation for previous vCont msg.
  while (!GDB::getInstance().isEmptyVContActionQueue(getCurrentCoreIdx())) {
    DBG::outlnISR(DBG::GDBDebug, "there is a pending operation.");
    // Report to GDB we stopped at current core.
    if (GDB::getInstance().nextVContAction(getCurrentCoreIdx())->executed) {
      DBG::outlnISR(DBG::GDBDebug, "pending operation done - ", *GDB::getInstance().nextVContAction(getCurrentCoreIdx()));
      GDB::getInstance().popVContAction(getCurrentCoreIdx());
      if (GDB::getInstance().isEmptyVContActionQueue(getCurrentCoreIdx())) {
        sendStopReply(outputBuffer, sigval);
        break;
      }
    } else {
      // TODO: should I send IPIs here for all-stop mode?
      DBG::outlnISR(DBG::GDBDebug, "entering consumeVContAction: ",
        *(GDB::getInstance().nextVContAction(getCurrentCoreIdx())));
      consumeVContAction();
    }
  }

begin:
  /**
   * All-Stop mode Only.
   *
   * Send INT 0x3 IPI to all available cores.
   */
  if (allstop) GDB::getInstance().sendIPIToAllOtherCores();
  entry_q.releaseISR();

  while (true) {
    outputBuffer[0] = 0;        // reset out buffer to 0
    memset(outputBuffer, 0, BUFMAX);
    ptr = getpacket ();         // get request from gdb

    switch (*ptr++) {
      case 'q':   // qOffsets, qSupported, qSymbol::
      {
        if (strncmp(ptr, "Supported", strlen("Supported")) == 0) {
          strcpy(outputBuffer, "PacketSize=3E8");  // max packet size = 1000 bytes
          break;
        } else if (*ptr == 'C') {
          strcpy(outputBuffer, "QC1");             // from QEMU's gdbstub.c
          break;
        } else if (strncmp(ptr, "Attached", strlen("Attached")) == 0) {
          outputBuffer[0] = 0;    // empty response, NOT on documentation
          break;
        } else if (strncmp(ptr, "Offsets", strlen("Offsets")) == 0) {
          strcpy(outputBuffer, "Text=0;Data=0;Bss=0");
          break;
        } else if (strncmp(ptr, "Symbol::", strlen("Symbol::")) == 0) {
          strcpy(outputBuffer, "OK");
          break;
        } else if (strncmp(ptr, "TStatus", strlen("TStatus")) == 0) {
          outputBuffer[0] = 0;                     // empty response works instead
          break;
        } else if (strncmp(ptr, "fThreadInfo", strlen("fThreadInfo")) == 0) {
          gdb::GDB::getInstance().startEnumerate();
          gdb::GdbCpuState* state = gdb::GDB::getInstance().next();
          strcpy(outputBuffer, state->getCpuInfo().c_str());
          break;
        } else if (strncmp(ptr, "sThreadInfo", strlen("sThreadInfo")) == 0) {
          gdb::GdbCpuState* state = gdb::GDB::getInstance().next();
          if (state) strcpy(outputBuffer, state->getCpuInfo().c_str());
          else strcpy(outputBuffer, "l");
          break;
        } else if (strncmp(ptr, "ThreadExtraInfo", strlen("ThreadExtraInfo")) == 0) {
          ptr += strlen("ThreadExtraInfo,");
          long cpuIndex;
          if (hexToInt(&ptr, &cpuIndex)) {
            char* cpuId = const_cast<char*>(gdb::GDB::getInstance()
                    .getCpuId(cpuIndex-1));
            mem2hex(cpuId, ptr, strlen(cpuId), 0);
            strcpy(outputBuffer, ptr);
            break;
          }
        }
        strcpy(outputBuffer, "qUnimplemented");     // catches unimplemented commands
      }
      break;
      case 'v':
        gdb_cmd_vresume(ptr, exceptionVector);
        break;
      case 'T':
      {
        long threadId;
        if (hexToInt(&ptr, &threadId)) {
          if (threadId >= -1 && threadId <= gdb::GDB::getInstance().getNumCpus()) {
            strcpy(outputBuffer, "OK");
          } else {
            strcpy(outputBuffer, "E05");
          }
          break;
        }
        strcpy(outputBuffer, "E05");
      }
      break;
      case 'H':   // Hc, Hg
      {
        char op = ptr[0];
        ptr++;
        long threadId;
        /**
         * threadId: -1 indicates all threads, 0 implies to pick any thread.
         */
        if (hexToInt(&ptr, &threadId)) {
          if (threadId > 0) {
            switch (op) {
              case 'g': GDB::getInstance().setGCpu(threadId-1); break;
              case 'c': GDB::getInstance().setCCpu(threadId-1); break;
              default: ABORT1("Invalid Hop"); break;
            }
          }
          strcpy(outputBuffer, "OK");
          break;
        }
        strcpy(outputBuffer, "HUnimplemented");     // catches unimplmented commands
      }
      break;
      case '?':
        outputBuffer[0] = 'S';
        outputBuffer[1] = hexchars[sigval >> 4];
        outputBuffer[2] = hexchars[sigval % 16];
        outputBuffer[3] = 0;
        break;
      case 'd':
        remote_debug = !(remote_debug);                 // toggle debug flag
        break;
      case 'g':       // return the value of the CPU registers
      {
        char* ptr = outputBuffer;
        GdbCpuState* gCpu = GDB::getInstance().getGCpu();
        ptr = mem2hex((char *) gCpu->getRegs64(), ptr, gdb::numRegs64 * sizeof(gdb::reg64), 0);
        ptr = mem2hex((char *) gCpu->getRegs32(), ptr, gdb::numRegs32 * sizeof(gdb::reg32), 0);
      }
      break;
      case 'G':       // set the value of the CPU registers - return OK
        hex2mem(ptr, (char *) GDB::getInstance().getGCpu()->getRegs64(), gdb::numRegs64 * sizeof(gdb::reg64), 0);
        hex2mem(ptr+gdb::numRegs64, (char *) GDB::getInstance().getGCpu()->getRegs32(), gdb::numRegs32 * sizeof(gdb::reg32), 0);
        strcpy (outputBuffer, "OK");
        break;
      case 'P':       // set the value of a single CPU register - return OK
      {
        long regno;
        if (hexToInt (&ptr, &regno) && *ptr++ == '=') {
          if (regno >= 0 && regno < gdb::numRegs64) {
            hex2mem (ptr, (char *) GDB::getInstance().getGCpu()->getReg64(regno), sizeof(gdb::reg64), 0);
            strcpy (outputBuffer, "OK");
            break;
          } else if (regno >= gdb::numRegs64 && regno < gdb::numRegs64 + gdb::numRegs32) {
            hex2mem(ptr, (char *) GDB::getInstance().getGCpu()->getReg32(regno-gdb::numRegs64), sizeof(gdb::reg32), 0);
            strcpy(outputBuffer, "OK");
            break;
          }
        }
        strcpy(outputBuffer, "OK");      // force orig_rax write to pass
      }
      break;
      // mAA..AA,LLLL  Read LLLL bytes at address AA..AA
      // TRY TO READ %x,%x.  IF SUCCEED, SET PTR = 0
      case 'm':
      {
        if (hexToInt (&ptr, &addr)) {
          if (*(ptr++) == ',') {
            if (hexToInt (&ptr, &length)) {
              ptr = 0;
              mem_err = 0;
              mem2hex ((char *) addr, outputBuffer, length, 1);
              if (mem_err) {
                strcpy (outputBuffer, "E03");
                debug_error ("memory fault");
              }
            }
          }
        }

        if (ptr) {
          strcpy (outputBuffer, "E01");
        }
      }
      break;
      // MAA..AA,LLLL: Write LLLL bytes at address AA.AA return OK
      // TRY TO READ '%x,%x:'.  IF SUCCEED, SET PTR = 0
      case 'M':
      {
        if (hexToInt (&ptr, &addr)) {
          if (*(ptr++) == ',') {
            if (hexToInt (&ptr, &length)) {
              if (*(ptr++) == ':') {
                mem_err = 0;
                hex2mem (ptr, (char *) addr, length, 1);
                if (mem_err) {
                  strcpy (outputBuffer, "E03");
                  debug_error ("memory fault");
                } else {
                  strcpy (outputBuffer, "OK");
                }
                ptr = 0;
              }
            }
          }
        }
        if (ptr) {
          strcpy (outputBuffer, "E02");
        }
      }
      break;
      // kill the program
      case 'k':       // do nothing
          break;
    } // switch

    // reply to the request
    putpacket(outputBuffer);
  }
}

// this function is used to set up exception handlers for tracing and breakpoints
void set_debug_traps(bool as) {
  exceptionHandler (0, catchException0);
  exceptionHandler (1, catchException1);
  exceptionHandler (3, catchException3);
  exceptionHandler (4, catchException4);
  exceptionHandler (5, catchException5);
  exceptionHandler (6, catchException6);
  exceptionHandler (7, catchException7);
  exceptionHandler (16, catchException16);

  waiting = new bool[GDB::getInstance().getNumCpus()];
  shouldReturnFromException = new bool[GDB::getInstance().getNumCpus()];
  coreIdxTable = new int[GDB::getInstance().getNumCpus()];
  threadsSeen = new ThreadSeen[GDB::getInstance().getNumCpus()];
  for (int i = 0; i < GDB::getInstance().getNumCpus(); i++) {
    waiting[i] = false;
    shouldReturnFromException[i] = false;
    coreIdxTable[i] = i;
  }

  allstop = as;
  initialized = 1;
}

// This function will generate a breakpoint exception.  It is used at the
// beginning of a program to sync up with a debugger and can be used
// otherwise as a quick means to stop program execution and "break" into
// the debugger
void breakpoint() {
  if (initialized) BREAKPOINT();
}
