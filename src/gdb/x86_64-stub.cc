/****************************************************************************

        THIS SOFTWARE IS NOT COPYRIGHTED

   HP offers the following for use in the public domain.  HP makes no
   warranty with regard to the software or it's performance and the
   user accepts the software "AS IS" with all faults.

   HP DISCLAIMS ANY WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD
   TO THIS SOFTWARE INCLUDING BUT NOT LIMITED TO THE WARRANTIES
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

****************************************************************************/

/****************************************************************************
 *  Header: remcom.c,v 1.34 91/03/09 12:29:49 glenne Exp $
 *
 *  Module name: remcom.c $
 *  Revision: 1.34 $
 *  Date: 91/03/09 12:29:49 $
 *  Contributor:     Lake Stevens Instrument Division$
 *
 *  Description:     low level support for gdb debugger. $
 *
 *  Considerations:  only works on target hardware $
 *
 *  Written by:      Glenn Engel $
 *  ModuleState:     Experimental $
 *
 *  NOTES:           See Below $
 *
 *  Modified for 386 by Jim Kingdon, Cygnus Support.
 *
 *  To enable debugger support, two things need to happen.  One, a
 *  call to set_debug_traps() is necessary in order to allow any breakpoints
 *  or error conditions to be properly intercepted and reported to gdb.
 *  Two, a breakpoint needs to be generated to begin communication.  This
 *  is most easily accomplished by a call to breakpoint().  Breakpoint()
 *  simulates a breakpoint by executing a trap #1.
 *
 *  The external function exceptionHandler() is
 *  used to attach a specific handler to a specific 386 vector number.
 *  It should use the same privilege level it runs at.  It should
 *  install it as an interrupt gate so that interrupts are masked
 *  while the handler runs.
 *
 *  Because gdb will sometimes write to the stack area to execute function
 *  calls, this program cannot rely on using the supervisor stack so it
 *  uses it's own stack area reserved in the int array remcomStack.
 *
 *************
 *
 *    The following gdb commands are supported:
 *
 * command          function                               Return value
 *
 *    g             return the value of the CPU registers  hex data or ENN
 *    G             set the value of the CPU registers     OK or ENN
 *
 *    mAA..AA,LLLL  Read LLLL bytes at address AA..AA      hex data or ENN
 *    MAA..AA,LLLL: Write LLLL bytes at address AA.AA      OK or ENN
 *
 *    c             Resume at current address              SNN   ( signal NN)
 *    cAA..AA       Continue at address AA..AA             SNN
 *
 *    s             Step one instruction                   SNN
 *    sAA..AA       Step one instruction from AA..AA       SNN
 *
 *    k             kill
 *
 *    ?             What was the last sigval ?             SNN   (signal NN)
 *
 * All commands and responses are sent with a packet which includes a
 * checksum.  A packet consists of
 *
 * $<packet info>#<checksum>.
 *
 * where
 * <packet info> :: <characters representing the command or response>
 * <checksum>    :: < two hex digits computed as modulo 256 sum of <packetinfo>>
 *
 * When a packet is received, it is first acknowledged with either '+' or '-'.
 * '+' indicates a successful transfer.  '-' indicates a failed transfer.
 *
 * Example:
 *
 * Host:                  Reply:
 * $m0,10#2a               +$00010203040506070809101112131415#42
 *
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>


// function stubs for GDB remote serial protocol
extern void putDebugChar(unsigned char ch);         // write a single character
extern int getDebugChar();                          // read and return a single char
extern void exceptionHandler(int, void (*)());      // assign an exception handler

#define BUFMAX 4096         // maximum number of characters in inbound/outbound buffers

static char initialized;    // boolean flag. != 0 means we've been initialized
int remote_debug;           //  debug > 0 prints ill-formed commands in valid packets & checksum errors

static const char hexchars[]="0123456789abcdef";

static const int NUM64BITREGS = 17;     // number of 64-bit registers
static const int NUM32BITREGS = 7;      // EFLAGS, CS, SS, DS, ES, FS, GS

// Number of bytes of registers
static const int NUM64BITREGBYTES = NUM64BITREGS * 8;
static const int NUM32BITREGBYTES = NUM32BITREGS * 4;

// gives regs64:: prefix to distinguish with 32bit registers enum
struct regs64 {
    enum { RAX, RBX, RCX, RDX, RSI, RDI, RBP, RSP,
           R8, R9, R10, R11, R12, R13, R14, R15, RIP };
};

// gives regs32:: prifx to distinguish with 32bit registers enum
struct regs32 {
    enum { EFLAGS, CS, SS, DS, ES, FS, GS };
};

// used by inline assembly code; cannot be declared static
uint64_t registers_64[NUM64BITREGS];
uint32_t registers_32[NUM32BITREGS];

// define a new stack
static const int STACKSIZE = 10000;
static uint64_t remcomStack[STACKSIZE/sizeof(uint64_t)];
uint64_t* stackPtr = &remcomStack[STACKSIZE/sizeof(uint64_t) - 1];


// Restore the program's registers (including the stack pointer, which
// means we get the right stack and don't have to worry about popping our
// return address and any stack frames and so on) and return.
// NOTE: CS, RIP, SS, RSP, EFLAGS registers are restored by IRETQ call
//       FS, GS registers are not modifed by GDB and must be set by wrmsr instruction
extern "C" void
return_to_prog ();
asm(".text");
asm(".globl return_to_prog");
asm("return_to_prog:");
asm("       movq registers_64+8,    %rbx");
asm("       movq registers_64+16,   %rcx");
asm("       movq registers_64+24,   %rdx");
asm("       movq registers_64+32,   %rsi");
asm("       movq registers_64+40,   %rdi");
asm("       movq registers_64+48,   %rbp");
asm("       movq registers_64+64,   %r8");
asm("       movq registers_64+72,   %r9");
asm("       movq registers_64+80,   %r10");
asm("       movq registers_64+88,   %r11");
asm("       movq registers_64+96,   %r12");
asm("       movq registers_64+104,  %r13");
asm("       movq registers_64+112,  %r14");
asm("       movq registers_64+120,  %r15");
asm("       movw registers_32+12,   %ds");
asm("       movw registers_32+16,   %es");
asm("       movq $0, %rax");                            // clear RAX
asm("       movl registers_32+8,    %eax");             // SS -> RAX
asm("       pushq %rax");                               // push SS
asm("       movq registers_64+56,   %rax");             // RSP -> RAX
asm("       pushq %rax");                               // push rsp
asm("       movq $0, %rax");                            // clear RAX
asm("       movl registers_32,      %eax");             // EFLAGS -> RAX
asm("       pushq %rax");                               // push eflags
asm("       movq $0, %rax");                            // claer RAX
asm("       movl registers_32+4,    %eax");             // CS -> RAX
asm("       pushq %rax");                               // push cs
asm("       movq registers_64+128,  %rax");             // RIP -> RAX
asm("       pushq %rax");                               // push rip
asm("       movq registers_64,      %rax");
// use iret to restore pc and flags together so
// that trace flag works right
// iretq requires
// SS
// RSP
// EFLAGS
// CS
// RIP         <-- top of stack
asm("        iretq");

#define BREAKPOINT() asm("int $3");

// Put the error code here just in case the user cares
int64_t gdb_errcode;
// Likewise, the vector number here (since GDB only gets the signal
// number through the usual means, and that's not very specific)
static int64_t gdb_vector = -1;

// Saves RAX, RBX, RCX, RDX, RSI, RDI, RBP, R8-R15, DS, ES, FS, GS
#define SAVE_REGISTERS1() \
  asm ("movq %rax, registers_64");                         \
  asm ("movq %rbx, registers_64+8");                       \
  asm ("movq %rcx, registers_64+16");                      \
  asm ("movq %rdx, registers_64+24");                      \
  /* save stack pointer later */                           \
  asm ("movq %rsi, registers_64+32");                      \
  asm ("movq %rdi, registers_64+40");                      \
  asm ("movq %rbp, registers_64+48");                      \
  /* r8-r15 */                                             \
  asm ("movq %r8, registers_64+64");                       \
  asm ("movq %r9, registers_64+72");                       \
  asm ("movq %r10, registers_64+80");                      \
  asm ("movq %r11, registers_64+88");                      \
  asm ("movq %r12, registers_64+96");                      \
  asm ("movq %r13, registers_64+104");                     \
  asm ("movq %r14, registers_64+112");                     \
  asm ("movq %r15, registers_64+120");                     \
  asm ("movl $0, %eax");                                \
  /* rip (pc) eflags (ps), cs, ss saved later */        \
  /* ds */                                              \
  asm ("movw %ds, registers_32+12");                    \
  asm ("movw %ax, registers_32+14");                    \
  /* es */                                              \
  asm ("movw %es, registers_32+16");                    \
  asm ("movw %ax, registers_32+18");                    \
  /* fs */                                              \
  asm ("movw %fs, registers_32+20");                    \
  asm ("movw %ax, registers_32+22");                    \
  /* gs */                                              \
  asm ("movw %gs, registers_32+24");                    \
  asm ("movw %ax, registers_32+26");

#define SAVE_ERRCODE() \
  asm ("popq %rbx");                                    \
  asm ("movq %rbx, gdb_errcode");

// RIP, CS, EFLAGS, RSP, SS registers saved here
#define SAVE_REGISTERS2() \
  /* old rip */                                         \
  asm ("popq %rbx");                                    \
  asm ("movq %rbx, registers_64+128");                  \
  /* old cs */                                          \
  asm ("popq %rbx");                                    \
  asm ("movl %ebx, registers_32+4");                    \
  /* old eflags */                                      \
  asm ("popq %rbx");                                    \
  asm ("movl %ebx, registers_32");                      \
  /* pop rsp and ss too from stack for 64-bit */        \
  /* rsp register */                                    \
  asm("popq %rbx");                                     \
  asm("movq %rbx, registers_64+56");                    \
  /* ss register */                                     \
  asm("popq %rbx");                                     \
  asm("movl %ebx, registers_32+8");

// See if mem_fault_routine is set, if so just IRET to that address
#define CHECK_FAULT() \
  asm ("cmpl $0, mem_fault_routine");                   \
  asm ("jne mem_fault");

asm (".text");
asm ("mem_fault:");
// OK to clobber temp registers; we're just going to end up in set_mem_err.
// Pop error code from the stack and save it.
asm ("     popq %rax");
asm ("     movq %rax, gdb_errcode");

asm ("     popq %rax"); /* rip */
// We don't want to return there, we want to return to the function
// pointed to by mem_fault_routine instead.
asm ("     movq mem_fault_routine, %rax");
asm ("     popq %rcx"); /* cs (low 16 bits; junk in hi 16 bits).  */
asm ("     popq %rdx"); /* eflags */

// Remove this stack frame; when we do the iret, we will be going to
// the start of a function, so we want the stack to look just like it
// would after a "call" instruction.
asm ("     leave");

// TODO: need to check if this part of code is okay
/* Push the stuff that iret wants.  */
asm ("     pushq %rdx"); /* eflags */
asm ("     pushq %rcx"); /* cs */
asm ("     pushq %rax"); /* eip */

/* Zero mem_fault_routine.  */
asm ("     movq $0, %rax");
asm ("     movq %rax, mem_fault_routine");

asm ("iret");

#define CALL_HOOK() asm("call _remcomHandler");

/* This function is called when a x86_64 exception occurs.  It saves
 * all the cpu regs in the registers array, munges the stack a bit,
 * and invokes an exception handler (remcom_handler).
 *
 * stack on entry:                       stack on exit:
 *   old ss
 *   old rsp
 *   old eflags                          vector number
 *   old cs (zero-filled to 32 bits)
 *   old rip
 *
 */
extern "C" void catchException3();
asm(".text");
asm(".globl catchException3");
asm("catchException3:");
SAVE_REGISTERS1();
SAVE_REGISTERS2();
asm ("pushq $3");
CALL_HOOK();

/* Same thing for exception 1.  */
extern "C" void catchException1();
asm(".text");
asm(".globl catchException1");
asm("catchException1:");
SAVE_REGISTERS1();
SAVE_REGISTERS2();
asm ("pushq $1");
CALL_HOOK();

/* Same thing for exception 0.  */
extern "C" void catchException0();
asm(".text");
asm(".globl catchException0");
asm("catchException0:");
SAVE_REGISTERS1();
SAVE_REGISTERS2();
asm ("pushq $0");
CALL_HOOK();

/* Same thing for exception 4.  */
extern "C" void catchException4();
asm(".text");
asm(".globl catchException4");
asm("catchException4:");
SAVE_REGISTERS1();
SAVE_REGISTERS2();
asm ("pushq $4");
CALL_HOOK();

/* Same thing for exception 5.  */
extern "C" void catchException5();
asm(".text");
asm(".globl catchException5");
asm("catchException5:");
SAVE_REGISTERS1();
SAVE_REGISTERS2();
asm ("pushq $5");
CALL_HOOK();

/* Same thing for exception 6.  */
extern "C" void catchException6();
asm(".text");
asm(".globl catchException6");
asm("catchException6:");
SAVE_REGISTERS1();
SAVE_REGISTERS2();
asm ("pushq $6");
CALL_HOOK();

/* Same thing for exception 7.  */
extern "C" void catchException7();
asm(".text");
asm(".globl catchException7");
asm("catchException7:");
SAVE_REGISTERS1();
SAVE_REGISTERS2();
asm ("pushq $7");
CALL_HOOK();

/* Same thing for exception 8.  */
extern "C" void catchException8();
asm(".text");
asm(".globl catchException8");
asm("catchException8:");
SAVE_REGISTERS1();
SAVE_ERRCODE();
SAVE_REGISTERS2();
asm ("pushq $8");
CALL_HOOK();

/* Same thing for exception 9.  */
extern "C" void catchException9();
asm(".text");
asm(".globl catchException9");
asm("catchException9:");
SAVE_REGISTERS1();
SAVE_REGISTERS2();
asm ("pushq $9");
CALL_HOOK();

/* Same thing for exception 10.  */
extern "C" void catchException10();
asm(".text");
asm(".globl catchException10");
asm("catchException10:");
SAVE_REGISTERS1();
SAVE_ERRCODE();
SAVE_REGISTERS2();
asm ("pushq $10");
CALL_HOOK();

/* Same thing for exception 12.  */
extern "C" void catchException12();
asm(".text");
asm(".globl catchException12");
asm("catchException12:");
SAVE_REGISTERS1();
SAVE_ERRCODE();
SAVE_REGISTERS2();
asm ("pushq $12");
CALL_HOOK();

/* Same thing for exception 16.  */
extern "C" void catchException16();
asm(".text");
asm(".globl catchException16");
asm("catchException16:");
SAVE_REGISTERS1();
SAVE_REGISTERS2();
asm ("pushq $16");
CALL_HOOK();

/* For 13, 11, and 14 we have to deal with the CHECK_FAULT stuff.  */

/* Same thing for exception 13.  */
extern "C" void catchException13 ();
asm (".text");
asm (".globl catchException13");
asm ("catchException13:");
CHECK_FAULT();
SAVE_REGISTERS1();
SAVE_ERRCODE();
SAVE_REGISTERS2();
asm ("pushq $13");
CALL_HOOK();

/* Same thing for exception 11.  */
extern "C" void catchException11 ();
asm (".text");
asm (".globl catchException11");
asm ("catchException11:");
CHECK_FAULT();
SAVE_REGISTERS1();
SAVE_ERRCODE();
SAVE_REGISTERS2();
asm ("pushq $11");
CALL_HOOK();

/* Same thing for exception 14.  */
extern "C" void catchException14 ();
asm (".text");
asm (".globl catchException14");
asm ("catchException14:");
CHECK_FAULT();
SAVE_REGISTERS1();
SAVE_ERRCODE();
SAVE_REGISTERS2();
asm ("pushq $14");
CALL_HOOK();

/*
 * remcomHandler is a front end for handle_exception.  It moves the
 * stack pointer into an area reserved for debugger use.
 */
asm("_remcomHandler:");
asm("           popq %rax");            /* pop off return address     */
asm("           popq %rax");            /* get the exception number   */
asm("       movq stackPtr, %rsp");      /* move to remcom stack area  */
asm("       movq %rax, %rdi");          /* pass exception as argument */
asm("       call  handle_exception");   /* this never returns */

void
_returnFromException()
{
    return_to_prog();
}

int
hex (char ch)
{
    if ((ch >= 'a') && (ch <= 'f'))
        return (ch - 'a' + 10);
    if ((ch >= '0') && (ch <= '9'))
        return (ch - '0');
    if ((ch >= 'A') && (ch <= 'F'))
        return (ch - 'A' + 10);
    return (-1);
}

static char inputBuffer[BUFMAX];             // input buffer
static char outputBuffer[BUFMAX];            // output buffer

// scan for the sequence $<data>#<checksum>
char *
getpacket ()
{
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
            if (ch == '$')
                goto retry;
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
void
putpacket (char *buffer)
{
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

void
debug_error (const char *format, ...)
{
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

void
set_mem_err ()
{
    mem_err = 1;
}

// These are separate functions so that they are so short and sweet
// that the compiler won't save any registers (if there is a fault
// to mem_fault, they won't get restored, so there better not be any
// saved)
int
get_char (char *addr)
{
    return *addr;
}

void
set_char (char *addr, int val)
{
    *addr = val;
}

// convert the memory pointed to by mem into hex, placing result in buf
// return a pointer to the last char put in buf (null)
// If MAY_FAULT is non-zero, then we should set mem_err in response to
// a fault; if zero treat a fault like any other fault in the stub
char *
mem2hex (char *mem, char *buf, int count, int may_fault)
{
    // HACK
    if (!mem) {
        for (int i = 0; i < count; i++) {
            *buf++ = '0';
            *buf++ = '0';
        }
        *buf = 0;
        return buf;
    }

    if (may_fault)
        mem_fault_routine = set_mem_err;
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
char *
hex2mem (char *buf, char *mem, int count, int may_fault)
{
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
int
computeSignal (int64_t exceptionVector)
{
    int sigval;
    switch (exceptionVector) {
    case 0:
      sigval = 8;
      break;            /* divide by zero */
    case 1:
      sigval = 5;
      break;            /* debug exception */
    case 3:
      sigval = 5;
      break;            /* breakpoint */
    case 4:
      sigval = 16;
      break;            /* into instruction (overflow) */
    case 5:
      sigval = 16;
      break;            /* bound instruction */
    case 6:
      sigval = 4;
      break;            /* Invalid opcode */
    case 7:
      sigval = 8;
      break;            /* coprocessor not available */
    case 8:
      sigval = 7;
      break;            /* double fault */
    case 9:
      sigval = 11;
      break;            /* coprocessor segment overrun */
    case 10:
      sigval = 11;
      break;            /* Invalid TSS */
    case 11:
      sigval = 11;
      break;            /* Segment not present */
    case 12:
      sigval = 11;
      break;            /* stack exception */
    case 13:
      sigval = 11;
      break;            /* general protection */
    case 14:
      sigval = 11;
      break;            /* page fault */
    case 16:
      sigval = 7;
      break;            /* coprocessor error */
    default:
      sigval = 7;       /* "software generated" */
    }
  return sigval;
}

// While we find nice hex chars, build an int
// return number of chars processed
int
hexToInt (char **ptr, long *intValue)
{
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

// distinguish if INT1 or INT3 is called
static int firstTime = 1;

// does all command processing for interfacing to gdb
extern "C" void
handle_exception (int64_t exceptionVector)
{
    int sigval, stepping;
    long addr, length;
    char *ptr;
    uint64_t newPC;

    gdb_vector = exceptionVector;

    if (remote_debug) {
        printf ("vector=%ld, sr=0x%x, pc=0x%lx\n",
            exceptionVector, registers_32[regs32::EFLAGS], registers_64[regs64::RIP]);
    }

    sigval = computeSignal(exceptionVector);
    ptr = outputBuffer;

    // When handling INT 1 (from EFLAGS set with TF flag, send some message first)
    if (exceptionVector == 1 || (!firstTime && (exceptionVector == 3))) {
        strcpy(outputBuffer, "S05");
        putpacket(outputBuffer);
    }
    firstTime = 0;

    stepping = 0;
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
                        outputBuffer[0] = 0;                     // empty response, NOT on documentation
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
                        strcpy(outputBuffer, "m1");
                        break;
                    } else if (strncmp(ptr, "sThreadInfo", strlen("sThreadInfo")) == 0) {
                        strcpy(outputBuffer, "l");
                        break;
                    } else if (strncmp(ptr, "ThreadExtraInfo", strlen("ThreadExtraInfo")) == 0) {
                        // TODO: incomplete yet
                        char* ptr = outputBuffer;
                        mem2hex("CPU#1 [running]", ptr, sizeof("CPU#1 [running]"), 0);
                        break;
                    }
                    strcpy(outputBuffer, "qUnimplemented");     // catches unimplemented commands
                }
                break;
            case 'H':   // Hc, Hg
                {
                    if (strncmp(ptr, "c-1", strlen("c-1")) == 0) {
                        strcpy(outputBuffer, "OK");             // allow step/continue to work on all threads
                        break;
                    } else if (strncmp(ptr, "c0", strlen("c0")) == 0) {
                        strcpy(outputBuffer, "OK");
                        break;
                    } else if (strncmp(ptr, "g-1", strlen("g-1")) == 0) {
                        strcpy(outputBuffer, "OK");             // all future opreations apply on all threads
                        break;
                    } else if (strncmp(ptr, "g0", strlen("g0")) == 0) {
                        strcpy(outputBuffer, "OK");             // no documentation
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
                    ptr = mem2hex((char *) registers_64, ptr, NUM64BITREGBYTES, 0);
                    ptr = mem2hex((char *) registers_32, ptr, NUM32BITREGBYTES, 0);
                }
                break;
            case 'G':       // set the value of the CPU registers - return OK
                hex2mem(ptr, (char *) registers_64, NUM64BITREGBYTES, 0);
                hex2mem(ptr+NUM64BITREGBYTES, (char *) registers_32, NUM32BITREGBYTES, 0);
                strcpy (outputBuffer, "OK");
                break;
            case 'P':       // set the value of a single CPU register - return OK
            {
                long regno;

                if (hexToInt (&ptr, &regno) && *ptr++ == '=') {
                    if (regno >= 0 && regno < NUM64BITREGS) {
                        hex2mem (ptr, (char *) &registers_64[regno], 8, 0);
                        strcpy (outputBuffer, "OK");
                        break;
                    } else if (regno >= NUM64BITREGS && regno < NUM64BITREGS+NUM32BITREGS) {
                        hex2mem(ptr, (char *) &registers_32[regno-NUM64BITREGS], 4, 0);
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
            // cAA..AA    Continue at address AA..AA(optional)
            // sAA..AA   Step one instruction from AA..AA(optional)
            case 's': stepping = 1;
            case 'c':
            // try to read optional parameter, pc unchanged if no parm
                if (hexToInt (&ptr, &addr)) {
                    registers_64[regs64::RIP] = addr;
                }

                // clear the trace bit
                registers_32[regs32::EFLAGS] &= 0xfffffeff;

                // set the trace bit if we're stepping
                if (stepping) {
                    registers_32[regs32::EFLAGS] |= 0x100;      // TF (bit 8) set to single-step mode
                }
                _returnFromException();
                break;

            // kill the program
            case 'k':       // do nothing
                break;
        } // switch

        // reply to the request
        putpacket(outputBuffer);
    }
}

// this function is used to set up exception handlers for tracing and
// breakpoints
void
set_debug_traps()
{
  stackPtr = &remcomStack[STACKSIZE / sizeof (uint64_t) - 1];

  exceptionHandler (0, catchException0);
  exceptionHandler (1, catchException1);
  exceptionHandler (3, catchException3);
  exceptionHandler (4, catchException4);
  exceptionHandler (5, catchException5);
  exceptionHandler (6, catchException6);
  exceptionHandler (7, catchException7);
  exceptionHandler (8, catchException8);
  exceptionHandler (9, catchException9);
  exceptionHandler (10, catchException10);
  exceptionHandler (11, catchException11);
  exceptionHandler (12, catchException12);
  exceptionHandler (13, catchException13);
//  exceptionHandler (14, catchException14);        // use default ISR_handler from KOS
  exceptionHandler (16, catchException16);

  initialized = 1;
}

// This function will generate a breakpoint exception.  It is used at the
// beginning of a program to sync up with a debugger and can be used
// otherwise as a quick means to stop program execution and "break" into
// the debugger

void
breakpoint()
{
    if (initialized)
        BREAKPOINT();
}
