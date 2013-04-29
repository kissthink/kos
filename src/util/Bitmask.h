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
#ifndef _Bitmask_h_
#define _Bitmask_h_ 1

#include "util/basics.h"
#include "mach/bitstrops.h"

class Bitmask {
  mword bits;
public:
  constexpr Bitmask( mword b = 0 ) : bits(b) {}
  void init() { bits = 0; }
  void set( mword n, bool doit = true ) {
    bits |= (mword(doit) << n);
  }
  void clear( mword n, bool doit = true ) {
    bits &= ~(mword(doit) << n);
  }
  void flip( mword n, bool doit = true ) {
    bits ^= (mword(doit) << n);
  }
  constexpr bool test( mword n ) const {
    return bits & pow2(n);
  }
  mword lsbc( mword n, mword a ) {
    return find_lsb_cond( bits & ~maskbits(n), a );
  }
  mword msbc( mword n, mword a ) {
    return find_msb_cond( bits & maskbits(n), a );
  }
};

#endif /* _Bitmask_h_ */
