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

/************************************************************************
 *
 * external low-level support routines
 */

extern void putDebugChar(unsigned char ch);	/* write a single character      */
extern int getDebugChar();	/* read and return a single char */
extern void exceptionHandler(int, void (*)());	/* assign an exception handler   */

/************************************************************************/
/* BUFMAX defines the maximum number of characters in inbound/outbound buffers*/
/* at least NUMREGBYTES*2 are needed for register packets */
#define BUFMAX 10000

static char initialized;  /* boolean flag. != 0 means we've been initialized */

int     remote_debug;
/*  debug >  0 prints ill-formed commands in valid packets & checksum errors */

static const char hexchars[]="0123456789abcdef";

/* Number of 64 bit registers.  */
#define NUM64BITREGS 17

/* Number of 32 bit registers. */
#define NUM32BITREGS 7      // EFLAGS, CS, SS, DS, ES, FS, GS

/* Number of bytes of registers.  */
#define NUMREGBYTES (NUM64BITREGS * 8)

uint32_t eflags, cs, ss, ds, es, fs, gs;

enum regnames {RAX, RBX, RCX, RDX, RSI, RDI, RBP, RSP,
        R8, R9, R10, R11, R12, R13, R14, R15,
        RIP,
        EFLAGS,
        CS, SS, DS, ES, FS, GS};

/*
 * these should not be static cuz they can be used outside this module
 */
uint64_t registers[NUM64BITREGS];

#define STACKSIZE 10000
uint64_t remcomStack[STACKSIZE/sizeof(uint64_t)];
/*static*/ uint64_t* stackPtr = &remcomStack[STACKSIZE/sizeof(uint64_t) - 1];

/***************************  ASSEMBLY CODE MACROS *************************/
/* 									   */

extern "C" void
return_to_prog ();

/* Restore the program's registers (including the stack pointer, which
   means we get the right stack and don't have to worry about popping our
   return address and any stack frames and so on) and return.  */
asm(".text");
asm(".globl return_to_prog");
asm("return_to_prog:");
asm("       movq registers+8,   %rbx");
asm("       movq registers+16,  %rcx");
asm("       movq registers+24,  %rdx");
asm("       movq registers+32,  %rsi");
asm("       movq registers+40,  %rdi");
asm("       movq registers+48,  %rbp");
/* stack pointer pushed to stack for 64 bit IRETQ */
asm("       movq registers+64,  %r8");
asm("       movq registers+72,  %r9");
asm("       movq registers+80,  %r10");
asm("       movq registers+88,  %r11");
asm("       movq registers+96,  %r12");
asm("       movq registers+104, %r13");
asm("       movq registers+112, %r14");
asm("       movq registers+120, %r15");
asm("       movw ss, %ss");
asm("       movw ds, %ds");
asm("       movw es, %es");
asm("       movw fs, %fs");
asm("       movw gs, %gs");
asm("       movq $0, %rax");
asm("       movw %ss, %ax");
asm("       pushq %rax");                           /* ss */
asm("       movq registers+56,  %rax");
asm("       pushq %rax");                           /* saved rsp */
asm("       movq $0, %rax");
asm("       movl eflags, %eax");
asm("       pushq %rax");                           /* saved eflags */
asm("       movq $0, %rax");
asm("       movw cs, %ax");
asm("       pushq %rax");                           /* saved cs */
asm("       movq registers+128, %rax");
asm("       pushq %rax");                           /* saved rip */
asm("       movq registers,     %rax");

/* use iret to restore pc and flags together so
   that trace flag works right.  */
asm("        iretq");

#define BREAKPOINT() asm("   int $3");

/* Put the error code here just in case the user cares.  */
int64_t gdb_x8664errcode;
/* Likewise, the vector number here (since GDB only gets the signal
   number through the usual means, and that's not very specific).  */
int64_t gdb_x8664vector = -1;

/* GDB stores segment registers in 32-bit words (that's just the way
   m-i386v.h is written).  So zero the appropriate areas in registers.  */
#define SAVE_REGISTERS1() \
  asm ("movq %rax, registers");                         \
  asm ("movq %rbx, registers+8");                       \
  asm ("movq %rcx, registers+16");                      \
  asm ("movq %rdx, registers+24");                      \
  /* save stack pointer later */                        \
  asm ("movq %rsi, registers+32");                      \
  asm ("movq %rdi, registers+40");                      \
  asm ("movq %rbp, registers+48");                      \
  /* r8-r15 */                                          \
  asm ("movq %r8, registers+64");       /* 64 */        \
  asm ("movq %r9, registers+72");       /* 64 */        \
  asm ("movq %r10, registers+80");      /* 64 */        \
  asm ("movq %r11, registers+88");      /* 64 */        \
  asm ("movq %r12, registers+96");      /* 64 */        \
  asm ("movq %r13, registers+104");     /* 64 */        \
  asm ("movq %r14, registers+112");     /* 64 */        \
  asm ("movq %r15, registers+120");                     \
  asm ("movl $0, %eax");                                \
  /* rip (pc) eflags (ps), cs, ss saved later */        \
  /* ds */                                              \
  asm ("movw %ds, ds");                 /* 32 */        \
  asm ("movw %ax, ds+2");                               \
  /* es */                                              \
  asm ("movw %es, es");                 /* 32 */        \
  asm ("movw %ax, es+2");                               \
  /* fs */                                              \
  asm ("movw %fs, fs");                 /* 32 */        \
  asm ("movw %ax, fs+2");                               \
  /* gs */                                              \
  asm ("movw %gs, gs");                 /* 32 */        \
  asm ("movw %ax, gs+2");
#define SAVE_ERRCODE() \
  asm ("popq %rbx");                                    \
  asm ("movq %rbx, gdb_x8664errcode");
#define SAVE_REGISTERS2() \
  /* old rip */                                         \
  asm ("popq %rbx");                                    \
  asm ("movq %rbx, registers+128");     /* 64 */        \
  /* old cs */                                          \
  asm ("popq %rbx");                                    \
  asm ("movl %ebx, cs");                                \
  /* old eflags */                                      \
  asm ("popq %rbx");                                    \
  asm ("movl %ebx, eflags");            /* 32 */        \
  /* pop rsp and ss too from stack for 64-bit */        \
  /* rsp register */                                    \
  asm("popq %rbx");                                     \
  asm("movq %rbx, registers+56");                       \
  /* ss register */                                     \
  asm("popq %rbx");                                     \
  asm("movl %ebx, ss");

/* See if mem_fault_routine is set, if so just IRET to that address.  */
#define CHECK_FAULT() \
  asm ("cmpl $0, mem_fault_routine");					   \
  asm ("jne mem_fault");

asm (".text");
asm ("mem_fault:");
/* OK to clobber temp registers; we're just going to end up in set_mem_err.  */
/* Pop error code from the stack and save it.  */
asm ("     popq %rax");
asm ("     movq %rax, gdb_x8664errcode");

asm ("     popq %rax"); /* rip */
/* We don't want to return there, we want to return to the function
   pointed to by mem_fault_routine instead.  */
asm ("     movq mem_fault_routine, %rax");
asm ("     popq %rcx"); /* cs (low 16 bits; junk in hi 16 bits).  */
asm ("     popq %rdx"); /* eflags */
asm ("     popq %rbx"); /* rsp */
asm ("     popq %rsi"); /* ss */

/* Remove this stack frame; when we do the iret, we will be going to
   the start of a function, so we want the stack to look just like it
   would after a "call" instruction.  */
asm ("     leave");

/* Push the stuff that iret wants.  */
asm ("     pushq %rsi"); /* ss */
asm ("     pushq %rbx"); /* rsp */
asm ("     pushq %rdx"); /* eflags */
asm ("     pushq %rcx"); /* cs */
asm ("     pushq %rax"); /* eip */

/* Zero mem_fault_routine.  */
asm ("     movq $0, %rax");
asm ("     movq %rax, mem_fault_routine");

asm ("iretq");

#define CALL_HOOK() asm("call _remcomHandler");

/* This function is called when a i386 exception occurs.  It saves
 * all the cpu regs in the registers array, munges the stack a bit,
 * and invokes an exception handler (remcom_handler).
 *
 * stack on entry:                       stack on exit:
 *   old eflags                          vector number
 *   old cs (zero-filled to 32 bits)
 *   old eip
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
asm("		movq stackPtr, %rsp");      /* move to remcom stack area  */
asm("       movq %rax, %rdi");          /* pass exception as argument */
asm("		call  handle_exception");   /* this never returns */

void
_returnFromException ()
{
  return_to_prog ();
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

static char remcomInBuffer[BUFMAX];
static char remcomOutBuffer[BUFMAX];

/* scan for the sequence $<data>#<checksum> */

char *
getpacket (void)
{
  char *buffer = &remcomInBuffer[0];
  char checksum;
  char xmitcsum;
  int count;
  char ch;

  while (1)
    {
      /* wait around for the start character, ignore all other characters */
      while ((ch = getDebugChar ()) != '$')
	;

    retry:
      checksum = 0;
      xmitcsum = -1;
      count = 0;

      /* now, read until a # or end of buffer is found */
      while (count < BUFMAX - 1)
	{
	  ch = getDebugChar ();
	  if (ch == '$')
	    goto retry;
	  if (ch == '#')
	    break;
	  checksum = checksum + ch;
	  buffer[count] = ch;
	  count = count + 1;
	}
      buffer[count] = 0;

      if (ch == '#')
	{
	  ch = getDebugChar ();
	  xmitcsum = hex (ch) << 4;
	  ch = getDebugChar ();
	  xmitcsum += hex (ch);

	  if (checksum != xmitcsum)
	    {
	      if (remote_debug)
		{
		  fprintf (stderr,
			   "bad checksum.  My count = 0x%x, sent=0x%x. buf=%s\n",
			   checksum, xmitcsum, buffer);
		}
	      putDebugChar ('-');	/* failed checksum */
	    }
	  else
	    {
	      putDebugChar ('+');	/* successful transfer */

	      /* if a sequence char is present, reply the sequence ID */
	      if (buffer[2] == ':')
		{
		  putDebugChar (buffer[0]);
		  putDebugChar (buffer[1]);

		  return &buffer[3];
		}

	      return &buffer[0];
	    }
	}
    }
}

/* send the packet in buffer.  */

void
putpacket (char *buffer)
{
  unsigned char checksum;
  int count;
  char ch;

  /*  $<packet info>#<checksum>.  */
  do
    {
      putDebugChar ('$');
      checksum = 0;
      count = 0;

      while ( (ch = buffer[count]) )
	{
	  putDebugChar (ch);
	  checksum += ch;
	  count += 1;
	}

      putDebugChar ('#');
      putDebugChar (hexchars[checksum >> 4]);
      putDebugChar (hexchars[checksum % 16]);

    }
  while (getDebugChar () != '+');
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

/* Address of a routine to RTE to if we get a memory fault.  */
/*static*/ void (*volatile mem_fault_routine) () = NULL;

/* Indicate to caller of mem2hex or hex2mem that there has been an
   error.  */
static volatile int mem_err = 0;

void
set_mem_err (void)
{
  mem_err = 1;
}

/* These are separate functions so that they are so short and sweet
   that the compiler won't save any registers (if there is a fault
   to mem_fault, they won't get restored, so there better not be any
   saved).  */
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

/* convert the memory pointed to by mem into hex, placing result in buf */
/* return a pointer to the last char put in buf (null) */
/* If MAY_FAULT is non-zero, then we should set mem_err in response to
   a fault; if zero treat a fault like any other fault in the stub.  */
char *
mem2hex (char *mem, char *buf, int count, int may_fault)
{
  int i;
  unsigned char ch;

  if (may_fault)
    mem_fault_routine = set_mem_err;
  for (i = 0; i < count; i++)
    {
      ch = get_char (mem++);
      if (may_fault && mem_err)
	return (buf);
      *buf++ = hexchars[ch >> 4];
      *buf++ = hexchars[ch % 16];
    }
    *buf = 0;
  if (may_fault)
    mem_fault_routine = NULL;

  return (buf);
}

/* convert the hex array pointed to by buf into binary to be placed in mem */
/* return a pointer to the character AFTER the last byte written */
char *
hex2mem (char *buf, char *mem, int count, int may_fault)
{
  int i;
  unsigned char ch;

  if (may_fault)
    mem_fault_routine = set_mem_err;
  for (i = 0; i < count; i++)
    {
      ch = hex (*buf++) << 4;
      ch = ch + hex (*buf++);
      set_char (mem++, ch);
      if (may_fault && mem_err)
	return (mem);
    }
  if (may_fault)
    mem_fault_routine = NULL;
  return (mem);
}

/* this function takes the 386 exception vector and attempts to
   translate this number into a unix compatible signal value */
int
computeSignal (int64_t exceptionVector)
{
  int sigval;
  switch (exceptionVector)
    {
    case 0:
      sigval = 8;
      break;			/* divide by zero */
    case 1:
      sigval = 5;
      break;			/* debug exception */
    case 3:
      sigval = 5;
      break;			/* breakpoint */
    case 4:
      sigval = 16;
      break;			/* into instruction (overflow) */
    case 5:
      sigval = 16;
      break;			/* bound instruction */
    case 6:
      sigval = 4;
      break;			/* Invalid opcode */
    case 7:
      sigval = 8;
      break;			/* coprocessor not available */
    case 8:
      sigval = 7;
      break;			/* double fault */
    case 9:
      sigval = 11;
      break;			/* coprocessor segment overrun */
    case 10:
      sigval = 11;
      break;			/* Invalid TSS */
    case 11:
      sigval = 11;
      break;			/* Segment not present */
    case 12:
      sigval = 11;
      break;			/* stack exception */
    case 13:
      sigval = 11;
      break;			/* general protection */
    case 14:
      sigval = 11;
      break;			/* page fault */
    case 16:
      sigval = 7;
      break;			/* coprocessor error */
    default:
      sigval = 7;		/* "software generated" */
    }
  return (sigval);
}

/**********************************************/
/* WHILE WE FIND NICE HEX CHARS, BUILD AN INT */
/* RETURN NUMBER OF CHARS PROCESSED           */
/**********************************************/
int
hexToInt (char **ptr, long *intValue)
{
  int numChars = 0;
  int hexValue;

  *intValue = 0;

  while (**ptr)
    {
      hexValue = hex (**ptr);
      if (hexValue >= 0)
	{
	  *intValue = (*intValue << 4) | hexValue;
	  numChars++;
	}
      else
	break;

      (*ptr)++;
    }

  return (numChars);
}

/*
 * This function does all command procesing for interfacing to gdb.
 */
extern "C" void
handle_exception (int64_t exceptionVector)
{
    int sigval, stepping;
    long addr, length;
    char *ptr;
    uint64_t newPC;

    gdb_x8664vector = exceptionVector;

    if (remote_debug) {
        printf ("vector=%ld, sr=0x%x, pc=0x%lx\n",
	        exceptionVector, eflags, registers[RIP]);
    }

    /* reply to host that an exception has occurred */
    sigval = computeSignal(exceptionVector);

    ptr = remcomOutBuffer;

    *ptr++ = 'T';			/* notify gdb with signo, RIP, FP and SP */
    /* hexchars 0123456789abcdef */
    *ptr++ = hexchars[sigval >> 4];
    *ptr++ = hexchars[sigval & 0xf];

    *ptr++ = hexchars[RSP]; 
    *ptr++ = ':';
    ptr = mem2hex((char *)&registers[RSP], ptr, 8, 0);	/* SP */
    *ptr++ = ';';

    *ptr++ = hexchars[RBP]; 
    *ptr++ = ':';
    ptr = mem2hex((char *)&registers[RBP], ptr, 8, 0); 	/* FP */
    *ptr++ = ';';

    *ptr++ = hexchars[RIP >> 4]; 
    *ptr++ = hexchars[RIP & 0xf];
    *ptr++ = ':';
    ptr = mem2hex((char *)&registers[RIP], ptr, 8, 0); 	/* RIP */
    *ptr++ = ';';

    *ptr = '\0';

    /* T:054:RSP;5:RBP;8:RIP */

    putpacket (remcomOutBuffer);

    stepping = 0;

    while (1 == 1) {
        remcomOutBuffer[0] = 0;     // reset out buffer to 0
        memset(remcomOutBuffer, 0, BUFMAX);
        ptr = getpacket ();         // get request from gdb

        switch (*ptr++) {
            case '?':
                remcomOutBuffer[0] = 'S';
                remcomOutBuffer[1] = hexchars[sigval >> 4];
                remcomOutBuffer[2] = hexchars[sigval % 16];
                remcomOutBuffer[3] = 0;
	            break;
            case 'd':
	            remote_debug = !(remote_debug);	/* toggle debug flag */
	            break;
            case 'g':		/* return the value of the CPU registers */
                {
                    char* ptr = remcomOutBuffer;
                    ptr = mem2hex ((char *) registers, ptr, NUMREGBYTES, 0);
                    ptr = mem2hex ((char *) &eflags, ptr, 4, 0);
                    ptr = mem2hex ((char *) &cs, ptr, 4, 0);
                    ptr = mem2hex ((char *) &ss, ptr, 4, 0);
                    ptr = mem2hex ((char *) &ds, ptr, 4, 0);
                    ptr = mem2hex ((char *) &es, ptr, 4, 0);
                    ptr = mem2hex ((char *) &fs, ptr, 4, 0);
                    ptr = mem2hex ((char *) &gs, ptr, 4, 0);
                }
	            break;
	        case 'G':		/* set the value of the CPU registers - return OK */
                hex2mem (ptr, (char *) registers, NUMREGBYTES, 0);
                hex2mem (ptr+NUMREGBYTES,    (char *)&eflags, 4, 0);
                hex2mem (ptr+NUMREGBYTES+4,  (char *)&cs, 4, 0);
                hex2mem (ptr+NUMREGBYTES+8,  (char *)&ss, 4, 0);
                hex2mem (ptr+NUMREGBYTES+12, (char *)&ds, 4, 0);
                hex2mem (ptr+NUMREGBYTES+16, (char *)&es, 4, 0);
                hex2mem (ptr+NUMREGBYTES+20, (char *)&fs, 4, 0);
                hex2mem (ptr+NUMREGBYTES+24, (char *)&gs, 4, 0);
                strcpy (remcomOutBuffer, "OK");
                break;
            case 'P':		/* set the value of a single CPU register - return OK */
	        {
	            long regno;

	            if (hexToInt (&ptr, &regno) && *ptr++ == '=') {
                    if (regno >= 0 && regno < NUM64BITREGS) {
                        hex2mem (ptr, (char *) &registers[regno], 8, 0);
                        strcpy (remcomOutBuffer, "OK");
                        break;
		            } else if (regno >= NUM64BITREGS && regno < 24) {
                        switch (regno) {
                            case EFLAGS:
                                hex2mem(ptr, (char *)&eflags, 4, 0);
                                break;
                            case CS:
                                hex2mem(ptr, (char *)&cs, 4, 0);
                                break;
                            case SS:
                                hex2mem(ptr, (char *)&ss, 4, 0);
                                break;
                            case DS:
                                hex2mem(ptr, (char *)&ds, 4, 0);
                                break;
                            case ES:
                                hex2mem(ptr, (char *)&es, 4, 0);
                                break;
                            case FS:
                                hex2mem(ptr, (char *)&fs, 4, 0);
                                break;
                            case GS:
                                hex2mem(ptr, (char *)&gs, 4, 0);
                                break;
                        }
                        strcpy(remcomOutBuffer, "OK");
                        break;
                    }
                }

                strcpy (remcomOutBuffer, "E01");
	        }
                break;
            /* mAA..AA,LLLL  Read LLLL bytes at address AA..AA */
            case 'm':
            /* TRY TO READ %x,%x.  IF SUCCEED, SET PTR = 0 */
            {
	            if (hexToInt (&ptr, &addr)) {
                    if (*(ptr++) == ',') {
	                    if (hexToInt (&ptr, &length)) {
		                    ptr = 0;
		                    mem_err = 0;
		                    mem2hex ((char *) addr, remcomOutBuffer, length, 1);
		                    if (mem_err) {
		                        strcpy (remcomOutBuffer, "E03");
		                        debug_error ("memory fault");
		                    }
		                }
                    }
                }

	            if (ptr) {
	                strcpy (remcomOutBuffer, "E01");
	            }
            }
	            break;
	        /* MAA..AA,LLLL: Write LLLL bytes at address AA.AA return OK */
	        case 'M':
	        /* TRY TO READ '%x,%x:'.  IF SUCCEED, SET PTR = 0 */
            {
	            if (hexToInt (&ptr, &addr)) {
	                if (*(ptr++) == ',') {
	                    if (hexToInt (&ptr, &length)) {
		                    if (*(ptr++) == ':') {
                                mem_err = 0;
                                hex2mem (ptr, (char *) addr, length, 1);

                                if (mem_err) {
                                    strcpy (remcomOutBuffer, "E03");
                                    debug_error ("memory fault");
                                } else {
                                    strcpy (remcomOutBuffer, "OK");
                                }

                                ptr = 0;
		                    }
                        }
                    }
                }
                if (ptr) {
                    strcpy (remcomOutBuffer, "E02");
                }
            }
	            break;
	        /* cAA..AA    Continue at address AA..AA(optional) */
	        /* sAA..AA   Step one instruction from AA..AA(optional) */
	        case 's':
	            stepping = 1;
	        case 'c':
	        /* try to read optional parameter, pc unchanged if no parm */
                if (hexToInt (&ptr, &addr)) {
                    registers[RIP] = addr;
                }

	            newPC = registers[RIP];

	            /* clear the trace bit */
                eflags &= 0xfffffeff;

	            /* set the trace bit if we're stepping */
	            if (stepping) {
                    eflags |= 0x100;
                }

	            _returnFromException ();	/* this is a jump */
	            break;

	        /* kill the program */
	        case 'k':		/* do nothing */
#if 0
	  /* Huh? This doesn't look like "nothing".
	     m68k-stub.c and sparc-stub.c don't have it.  */
	  BREAKPOINT ();
#endif
	            break;
	    }			/* switch */

        /* reply to the request */
        putpacket(remcomOutBuffer);
    }
}

/* this function is used to set up exception handlers for tracing and
   breakpoints */
void
set_debug_traps (void)
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
//  exceptionHandler (14, catchException14);
  exceptionHandler (16, catchException16);

  initialized = 1;
}

/* This function will generate a breakpoint exception.  It is used at the
   beginning of a program to sync up with a debugger and can be used
   otherwise as a quick means to stop program execution and "break" into
   the debugger.  */

void
breakpoint (void)
{
  if (initialized)
    BREAKPOINT ();
}
