/******************************************************************************
    Copyright 2012-2013 Martin Karsten

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
#ifndef _platform_h_
#define _platform_h_ 1

#include <cstddef>
#include <cstdint>

#if defined(__x86_64__)

typedef uint64_t mword;
typedef mword vaddr;
typedef mword laddr;
static const mword mwordbits = 64;
static const mword mwordlimit = ~mword(0);
static const mword topaddr = mwordlimit;

extern void Breakpoint2(vaddr ia = 0) __attribute__((noinline));

static inline void Breakpoint(vaddr ia = 0) {
  asm volatile( "xchg %%bx, %%bx" ::: "memory" );
  Breakpoint2(ia);
}

extern void Reboot(vaddr = 0);

static inline void out8( uint16_t port, uint8_t val ) {
  asm volatile("outb %0, %1" :: "a"(val), "Nd"(port) : "memory");
}

static inline uint8_t in8( uint16_t port ) {
  uint8_t ret;
  asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port) : "memory");
  return ret;
}

static inline void out16( uint16_t port, uint16_t val ) {
  asm volatile("outw %0, %1" :: "a"(val), "Nd"(port) : "memory");
}

static inline uint16_t in16( uint16_t port ) {
  uint16_t ret;
  asm volatile("inw %1, %0" : "=a"(ret) : "Nd"(port) : "memory");
  return ret;
}

static inline void out32( uint16_t port, uint32_t val ) {
  asm volatile("outl %0, %1" :: "a"(val), "Nd"(port) : "memory");
}

static inline uint32_t in32( uint16_t port ) {
  uint32_t ret;
  asm volatile("inl %1, %0" : "=a"(ret) : "Nd"(port) : "memory");
  return ret;
}

static inline void Nop() {
  asm volatile("nop" ::: "memory" );
}

static inline void Pause() {
  asm volatile("pause" ::: "memory");
}

static inline void Halt() {
  asm volatile("hlt" ::: "memory");
}

static inline void StoreBarrier() {
  asm volatile("sfence" ::: "memory");
}

static inline void LoadBarrier() {
  asm volatile("lfence" ::: "memory");
}

static inline void MemoryBarrier() {
  asm volatile("mfence" ::: "memory");
}

#else
#error unsupported architecture: only __x86_64__ supported at this time
#endif

#endif /* _platform_h_ */
