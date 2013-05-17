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
#ifndef _Debug_h_
#define _Debug_h_ 1

#include "util/Bitmask.h"
#include "util/SpinLock.h"

class DBG {
public:
  enum Level : size_t {
    Acpi = 0,
    AllStopGDB,
    Boot,
    Basic,
    Libc,
    Devices,
    Error,
    Frame,
    GDB,
    MemAcpi,
    Paging,
    PCI,
    Scheduler,
    VM,
    Warning,
    MaxLevel
  };

private:
  static SpinLock lk;

  static Bitmask levels;

  template<typename T>
  static void print( const T& msg ) {
    kcout << msg;
    kcdbg << msg;
  }

  template<typename T, typename... Args>
  static void print( const T& msg, const Args&... a ) {
    kcout << msg;
    kcdbg << msg;
    if (sizeof...(a)) print(a...);
  }

public:
  static void init( char* dstring, bool msg );

  template<typename... Args>
  static void out( Level c, const Args&... a ) {
    if (levels.test(c)) print(a...);
  }

  template<typename... Args>
  static void outln( Level c, const Args&... a ) {
    lk.acquire();
    out(c, a...);
    out(c, kendl);
    lk.release();
  }

  static bool test( Level c ) {
    return levels.test(c);
  }
};

#endif /* _Debug_h_ */
