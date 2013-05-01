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
#include "mach/platform.h"

class Bitmask {
  static_assert(mword(true) == mword(1), "true != mword(1)");
  mword bits;
public:
  constexpr Bitmask( mword b = 0 ) : bits(b) {}
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
  mword find( mword n = 0 ) {
    return lsbcond( bits & ~maskbits(n) );
  }
};

#endif /* _Bitmask_h_ */
