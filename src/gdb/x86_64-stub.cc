#include <cstring>
#include <cstdarg>
#include <cstdint>

#include "dev/Serial.h"
#include "gdb/GdbCpu.h"
#include "gdb/Gdb.h"
#include "gdb/gdb_asm_functions.h"
#include "gdb/NonBlockSemaphore.h"
#include "gdb/VContAction.h"
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

extern "C" void handle_exception(int64_t);

static const int BUFMAX = 4096;             // max number of chars in inbound/outbound buffers
static char initialized;                    // boolean flag. != 0 means we've been initialized

static const char hexchars[]="0123456789abcdef";

#define BREAKPOINT() asm("int $3");

static atomic<bool>isFree(true);            // baton is not picked up.
static atomic<int>lockHolder(-1);           // used for holding baton.
static bool* waiting;                       // represents a CPU waiting on entry queue.
static bool* shouldReturnFromException;     // return from interrupt handler right away (used for allstop mode)
static NonBlockSemaphore entry_q;
static GdbCpu** gCpu = nullptr;

static EmbeddedQueue<VContAction> vContActionQueue;
static Thread* gdbThread = nullptr;         // thread talking to gdb (remember because it moves among cores)
static VContAction* prevAction = nullptr;   // previous vContAction

static bool allstop = false;
void *mem_fault_return_addr = nullptr;

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

void __returnFromException() {
  Gdb::setCpuState(cpuState::RUNNING);
  if (allstop) {    // wake other threads if in all-stop mode
    int threadId = Processor::getApicID();
    for (int i = 0; i < Gdb::getNumCpusInitialized(); i++) {
      if (i == threadId) continue;
      if (waiting[i]) {
        waiting[i] = false;
        shouldReturnFromException[i] = true;
        Gdb::V(i);
      }
    }
  }
}

// get out of interrupt handler
void _returnFromException() {
  __returnFromException();
//  entry_q.releaseISR();
  restoreRegisters(Gdb::getCurrentCpu());
}

// use if entry_q is not locked
void _returnFromExceptionLocked() {
  __returnFromException();
  entry_q.V();
  restoreRegisters(Gdb::getCurrentCpu());
}

int hex(char ch) {
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

/**
 * Routines for access/modifying memory
 */
volatile int gdbFaultHandlerSet = 0;  // can activate fault handler for gdb session
volatile int mem_err = 0;             // flags an error from mem2hex/hex2mem
// put mem to buf in hex format and return next buf position
char* mem2hex (char *mem, char *buf, int count, int may_fault) {
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
int computeSignal (int64_t exceptionVector) {
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
  int numChars = 0;
  int hexValue;
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

  DBG::outln(DBG::GDBDebug, "Cleared TF bit for thread ",
    Processor::getApicID() + 1, " eflags: ", FmtHex(eflags));
}
void setTFBit() {
  reg32 eflags = Gdb::getReg32(registers::EFLAGS);
  eflags &= 0xfffffeff;
  eflags |= 0x100;
  Gdb::setReg32(registers::EFLAGS, eflags);

  DBG::outln(DBG::GDBDebug, "Set TF bit for thread ",
    Processor::getApicID() + 1, " eflags: ", FmtHex(eflags));
}
void clearTFBit(int cpuId) {
  reg32 eflags = Gdb::getReg32(registers::EFLAGS, cpuId);
  eflags &= 0xfffffeff;
  Gdb::setReg32(registers::EFLAGS, eflags, cpuId);
}
void setTFBit(int cpuId) {
  reg32 eflags = Gdb::getReg32(registers::EFLAGS, cpuId);
  eflags &= 0xfffffeff;
  eflags |= 0x100;
  Gdb::setReg32(registers::EFLAGS, eflags, cpuId);
}

// stop reply for vCont packets
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

  DBG::outln(DBG::GDBDebug, "Sending stop reply: ", outputBuffer,
    " from thread ", threadId);
  putpacket(outputBuffer);
}

// parse and queue vContActions (referenced QEMU's gdbstub)
// assem only 1 action is given from each vCont packet (QEMU does this)
VContAction* parse_vcont(char* ptr)
{
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
    if (res == 0 || (res == 'c' && action == 's')) {
      res = action;
      res_signal = signal;
      res_thread = threadId;
      DBG::outln(DBG::GDBDebug, "action: ", (char)res, " signal: ", res_signal,
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
      KASSERT1(res_thread <= Gdb::getNumCpusInitialized(), res_thread);
      vContAction->threadId = res_thread;
      vContAction->threadAssigned = true;
    }
    vContActionQueue.push(vContAction);
    result = vContAction;
    DBG::outln(DBG::GDBDebug, "Parsed vContAction - ", *vContAction);
  }

  return result;
}

// Consumes and executes vContAction for the current thread.
// Precondition: entry_q lock must be held.
void consumeVContAction() {
  VContAction* action = vContActionQueue.front();
  DBG::outln(DBG::GDBDebug, *action, " from thread: ", Processor::getApicID() + 1);
  KASSERT0(action);
  KASSERT1(!action->executed, action->action);
  KASSERT0(action->isForCurrentThread());

  clearTFBit();
  if (action->action[0] == 's') setTFBit();
  DBG::outln(DBG::GDBDebug, "Consuming vContAction - ", *action, " isFree ", isFree);

  lockHolder = Processor::getApicID();    // keep entry lock
  if (action->action[0] == 'c') {         // could be from step or next
    if (prevAction && prevAction->action[0] == 's') {   // do not switch thread for 'next'
      isFree = false;                     // hold on to the baton
      action->executed = true;            // generate stop reply packet after 'next'
      _returnFromExceptionLocked();       // do 'next'
    }
    isFree = true;                        // free baton
    // increment rip only if it was actual breakpoint set by user (not set by gdb for next)
    if (prevAction == nullptr || prevAction->action[0] != 'c') Gdb::incrementRip();
    // if there's a thread waiting on breakpoint, switch to that thread.
    // TODO: smarter way of choosing a thread
    for (int i = 0; i < Gdb::getNumCpusInitialized(); i++) {
      if (waiting[i]) {
        isFree = false;
        waiting[i] = false;
        Gdb::V(i);
        DBG::outln(DBG::GDBDebug, "Passed baton from thread: ",
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
  _returnFromExceptionLocked();
}

// Handles 'vCont' commands.
// TODO: support vAttach, vFile, vFlashErase, vFlashWrite, vFlashDone, vKill, vRun, vStopped.
void gdb_cmd_vresume(char* ptr, int64_t exceptionVector)
{
  if (strncmp(ptr, "Cont", strlen("Cont")) == 0) {
    ptr += strlen("Cont");
    if (*ptr == '?') {    // vCont? requests a list of actions supported by 'vCont' packet
      strcpy(outputBuffer, "vCont;c;C;s;S");
    } else {
      // vCont[;action[:thread-id]]...
      // FIXME: currently assumes only one action per vCont
      // (referenced QEMU 1.4.1, gdbstub.c)
      VContAction* action = parse_vcont(ptr);
      KASSERT0(action);
      entry_q.P();
      // if 'vCont' packet is not for current thread, wake the target thread and block
      if (!action->isForCurrentThread()) {
        KASSERT0(waiting[action->threadId-1]);  // target thread for 'c' or 's' must be waiting in interrupt handler
        if (waiting[action->threadId-1]) {
          DBG::outln(DBG::GDBDebug, "baton passed from ", Processor::getApicID()+1, " to ", action->threadId);
          Gdb::V(action->threadId-1);    // pass baton
          handle_exception(exceptionVector);    // block current thread
        }
      } else {
        isFree = false;                         // hold on to the baton
        consumeVContAction();
      }
    }
  }
}

// does all command processing for interfacing to gdb
extern "C" void
handle_exception (int64_t exceptionVector) {
  int sigval;
  long addr, length;
  char *ptr;

  KASSERT0(!Processor::interruptsEnabled());

  int threadId = Processor::getApicID();
  Gdb::setCpuState(cpuState::BREAKPOINT);

  Thread* currThread = Processor::getCurrThread();

  DBG::outln(DBG::GDBDebug, "thread=", threadId+1, ", vector=", exceptionVector,
            ", eflags=", FmtHex(Gdb::getReg32(registers::EFLAGS)),
            ", pc=", FmtHex(Gdb::getReg64(registers::RIP)),
            ", thread=", FmtHex(currThread));

  // for int 3, rip pushed by interrupt handling mechanism may not be a correct value
  // decrement rip value back to an instruction causing breakpoint and let it try again with a valid address
  Gdb::resetRip();
  if (exceptionVector == 3) {
    reg32 eflags = Gdb::getReg32(registers::EFLAGS);
    DBG::outln(DBG::GDBDebug, "eflags & 0x100 = ", (eflags & 0x100));
    if ((eflags & 0x100) == 0) {   // if TF is set, treat the rip valid
      Gdb::decrementRip();
      DBG::outln(DBG::GDBDebug, "thread ", threadId+1, " decrement rip to ",
        FmtHex(Gdb::getReg64(registers::RIP)));
    }
  }

  sigval = computeSignal(exceptionVector);
  ptr = outputBuffer;

  /**
   * If a baton is held by another thread and the lockholder is not you,
   * you have to wait until someone passes a baton.
   */
  DBG::outln(DBG::GDBDebug, "thread ", threadId+1, " trying to enter");
  entry_q.P();

  if (gdbThread != nullptr && gdbThread != currThread && !isFree && (lockHolder == -1 || lockHolder != threadId)) {
    waiting[threadId] = true;
    DBG::outln(DBG::GDBDebug, "thread ", threadId+1, " is waiting for real lock holder: ", lockHolder+1);
    entry_q.V();
    Gdb::P(threadId);
    DBG::outln(DBG::GDBDebug, "thread ", threadId+1, " woke up");
    if (shouldReturnFromException[threadId]) {
      DBG::outln(DBG::GDBDebug, "thread ", threadId+1, " returning from exception");
      shouldReturnFromException[threadId] = false;
      GdbCpu* state = Gdb::getCurrentCpu();
      state->setCpuState(cpuState::RUNNING);
      restoreRegisters(state);
    }
  }

  // Got hold of a baton. You are free to enter.
  isFree = false;
  waiting[threadId] = false;
  lockHolder = threadId;
  gdbThread = currThread;
  DBG::outln(DBG::GDBDebug, "thread ", threadId+1, " got in");

  memset(outputBuffer, 0, BUFMAX);      // reset output buffer

  if (!vContActionQueue.empty()) {
    DBG::outln(DBG::GDBDebug, "there is a pending operation.");
    VContAction* action = vContActionQueue.front();
    if (action->executed) {
      DBG::outln(DBG::GDBDebug, "pending operation done - ", *action);
      prevAction = action;              // remember previous action (if 'c' don't increment rip)
      vContActionQueue.pop();
      KASSERT0(vContActionQueue.empty());
      sendStopReply(outputBuffer, sigval);
    } else {
      DBG::outln(DBG::GDBDebug, "entering consumeVContAction: ", *action);
      consumeVContAction();
    }
  }

  /**
   * All-Stop mode Only.
   *
   * Send INT 0x3 IPI to all available cores.
   */
  if (allstop) Gdb::sendIPIToAllOtherCores(0x1);
  entry_q.V();

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
          Gdb::startEnumerate();
          GdbCpu* state = Gdb::next();
          strcpy(outputBuffer, state->getCpuIdxStr());
          break;
        } else if (strncmp(ptr, "sThreadInfo", strlen("sThreadInfo")) == 0) {
          GdbCpu* state = Gdb::next();
          if (state) strcpy(outputBuffer, state->getCpuIdxStr());
          else strcpy(outputBuffer, "l");
          break;
        } else if (strncmp(ptr, "ThreadExtraInfo", strlen("ThreadExtraInfo")) == 0) {
          ptr += strlen("ThreadExtraInfo,");
          long cpuIndex;
          if (hexToInt(&ptr, &cpuIndex)) {
            char* cpuId = const_cast<char*>(Gdb::getCpuName(cpuIndex-1));
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
          if (threadId >= -1 && threadId <= Gdb::getNumCpus()) {
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
              case 'g': setGCpu(threadId-1); break;
              case 'c': break;
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
      case 'g':       // return the value of the CPU registers
      {
        char* ptr = outputBuffer;
        GdbCpu* gCpu = getGCpu();
        ptr = mem2hex((char *) gCpu->getRegs64(), ptr, numRegs64 * sizeof(reg64), 0);
        ptr = mem2hex((char *) gCpu->getRegs32(), ptr, numRegs32 * sizeof(reg32), 0);
      }
      break;
      case 'G':       // set the value of the CPU registers - return OK
      {
        GdbCpu* gCpu = getGCpu();
        hex2mem(ptr, (char *) gCpu->getRegs64(), numRegs64 * sizeof(reg64), 0);
        hex2mem(ptr+numRegs64, (char *) gCpu->getRegs32(), numRegs32 * sizeof(reg32), 0);
        strcpy (outputBuffer, "OK");
      }
        break;
      case 'P':       // set the value of a single CPU register - return OK
      {
        long regno;
        GdbCpu* gCpu = getGCpu();
        if (hexToInt (&ptr, &regno) && *ptr++ == '=') {
          if (regno >= 0 && regno < numRegs64) {
            hex2mem (ptr, (char *) gCpu->getReg64(regno), sizeof(reg64), 0);
            strcpy (outputBuffer, "OK");
            break;
          } else if (regno >= numRegs64 && regno < numRegs64 + numRegs32) {
            hex2mem(ptr, (char *) gCpu->getReg32(regno-numRegs64), sizeof(reg32), 0);
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
  for (int i = 0; i < Gdb::getNumCpus(); i++) {
    waiting[i] = false;
    shouldReturnFromException[i] = false;
    gCpu[i] = nullptr;
  }
  entry_q.resetCounter(1);  // simulate mutex

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
