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

#if defined(__clang__)

static inline constexpr mword msb_recursive( mword x ) {
  return x == 1 ? 0 : 1 + msb_recursive(x >> 1);
}

static inline constexpr mword lsb_recursive( mword x ) {
  return x & 1 ? 0 : 1 + lsb_recursive(x >> 1);
}

static inline constexpr mword floorlog2( mword x ) {
  return x == 0 ? mwordbits : msb_recursive(x);
}

static inline constexpr mword ceilinglog2( mword x ) {
  return x == 1 ? 0 : msb_recursive(x - 1) + 1;
}

static inline constexpr mword bitalignment( mword x ) {
  return x == 0 ? mwordbits : lsb_recursive(x);
}

static inline constexpr mword lsbcond(mword x, mword alt = mwordbits) {
  return x == 0 ? alt : lsb_recursive(x);
}

static inline constexpr mword msbcond(mword x, mword alt = mwordbits) {
  return x == 0 ? alt : msb_recursive(x);
}

#else /* assume gcc */

static inline constexpr mword floorlog2( mword x ) {
  return x == 0 ? mwordbits : (mwordbits - __builtin_clzll(x)) - 1;
}

static inline constexpr mword ceilinglog2( mword x ) {
  return x == 1 ? 0 : (mwordbits - __builtin_clzll(x - 1));
}

static inline constexpr mword bitalignment( mword x ) {
  return x == 0 ? mwordbits : __builtin_ctzll(x);
}

static inline mword lsbcond(mword x, mword alt = mwordbits) {
  mword ret;
  asm volatile("bsfq %1, %0" : "=a"(ret) : "rm"(x));
#ifdef  __OPTIMIZE__
  // WITCHCRAFT ALERT: Unclear why "d" resp. "rm" are specifically needed here
  asm volatile("cmovzq %1, %0" : "=a"(ret) : "d"(alt));
#else
  asm volatile("cmovzq %1, %0" : "=a"(ret) : "rm"(alt));
#endif
  return ret;
}

static inline mword msbcond(mword x, mword alt = mwordbits) {
  mword ret;
  asm volatile("bsrq %1, %0" : "=a"(ret) : "rm"(x));
#ifdef  __OPTIMIZE__
  // WITCHCRAFT ALERT: Unclear why "d" resp. "rm" are specifically needed here
  asm volatile("cmovzq %1, %0" : "=a"(ret) : "d"(alt));
#else
  asm volatile("cmovzq %1, %0" : "=a"(ret) : "rm"(alt));
#endif
  return ret;
}

#endif

static inline mword floorlog2_alt( mword x ) {
  return msbcond(x, mwordbits);
}

static inline mword ceilinglog2_alt( mword x ) {
  return msbcond(x - 1, -1) + 1; // x = 1 -> msb = -1 (alt) -> result is 0
}

static inline mword bitalignment_alt( mword x ) {
  return lsbcond(x, mwordbits);
}

template<size_t N, bool free = false>
static inline mword multiscan(mword* data) { // assume loop unroll; no branch
  static_assert( N < mwordbits / ceilinglog2(mwordbits), "template parameter N too large" );
  mword result = 0;
  mword mask = ~0;
  for (size_t i = 0; i < N; i++) {
    mword scan = free ? ~data[i] : data[i];
    mword found;
#if 1
    asm volatile("bsfq %1, %0" : "=a"(scan) : "rm"(scan));
    asm volatile("setnz %%dl" ::: "rdx");
    asm volatile("movzbq %%dl, %0" : "=d"(found));
    result += (scan & mask);
    mask = mask & (found - 1);
    result += (mwordbits & mask);
#else
    scan = lsbcond(scan, mwordbits);
    found = bool(scan ^ mwordbits);
    result += (scan & mask);
    mask = mask & (found - 1);
#endif
  }
  return result;
}

#else

#error unsupported architecture: only __x86_64__ supported at this time

#endif

#endif /* _platform_h_ */
