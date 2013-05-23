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
#include "util/OutputSafe.h"

class DBG {
public:
  enum Level : size_t {
    Acpi = 0,
    Boot,
    Basic,
    Libc,
    Devices,
    Error,
    Frame,
    GDBAllStop,
    GDBDebug,
    GDBEnable,
    MemAcpi,
    Paging,
    PCI,
    Scheduler,
    VM,
    Warning,
    MaxLevel
  };

private:
  static Bitmask levels;

public:
  static void init( char* dstring, bool msg );

  template<typename... Args>
  static void out( Level c, const Args&... a ) {
    if (levels.test(c)) {
      StdDbg.out(a...);
      StdOut.out(a...);
    }
  }

  template<typename... Args>
  static void outln( Level c, const Args&... a ) {
    if (levels.test(c)) {
      StdDbg.outln(a...);
      StdOut.outln(a...);
    }
  }

  static bool test( Level c ) {
    return levels.test(c);
  }
};

#if defined(KASSERTN)
#error macro collision: KASSERTN
#endif
#define KASSERTN(expr,args...)  { if unlikely(!(expr)) { kassertprint( "KASSERT: " #expr " in " __FILE__ ":", __LINE__, __func__); StdErr.outln(args); StdDbg.outln(args); Reboot(); } }

#endif /* _Debug_h_ */
