/******************************************************************************
    Copyright 2012 Martin Karsten

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
#ifndef _bitstrops_h_
#define _bitstrops_h_ 1

#include "mach/platform.h"

static inline mword find_lsb_cond( mword x, smword alt ) {
  // if ( x != 0 ) return lsb(x); else return alt
  mword ret;
  asm volatile("bsfq %1, %0" : "=a"(ret) : "c"(x) : "memory");
  asm volatile("cmovz %1, %0" : "=a"(ret) : "c"(alt) : "memory");
  return ret;
}

static inline mword find_msb_cond( mword x, mword alt ) {
  // if ( x != 0 ) return msb(x); else return alt
  mword ret;
  asm volatile("bsrq %1, %0" : "=a"(ret) : "c"(x) : "memory");
  asm volatile("cmovz %1, %0" : "=a"(ret) : "c"(alt) : "memory");
  return ret;
}

static inline mword floorlog2( mword x ) {
  return find_msb_cond(x, mwordbits);
}

static inline mword ceilinglog2( mword x ) {
  // msb(-1) -> mwordbits => result is mwordbits + 1
  return find_msb_cond(x-1, -1) + 1;
}

static inline mword bitalignment( mword x ) {
  return find_lsb_cond(x, mwordbits);
}

#endif /* _bitstrops_h_ */
