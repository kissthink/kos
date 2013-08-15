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
#ifndef _Output_h_
#define _Output_h_ 1

#include "util/Output.h"
#include "ipc/SpinLock.h"
#include "extern/stl/outputbuf.h"

#include <ostream>
#include <iomanip>
#include <cstdarg>

class KernelOutput {
  ostream os;
  SpinLock lock;

  template<typename T>
  void print( const T& msg ) {
    os << msg;
  }

  template<typename T, typename... Args>
  void print( const T& msg, const Args&... a ) {
    os << msg;
    if (sizeof...(a)) print(a...);
  }

public:
  KernelOutput( OutputBuffer<char>& ob );

  template<typename... Args>
  void out( const Args&... a ) {
    ScopedLock<> sl(lock);
    print(a...);
  }

  template<typename... Args>
  void outln( const Args&... a ) {
    ScopedLock<> sl(lock);
    print(a...);
    print(kendl);
  }
};

extern KernelOutput StdOut;
extern KernelOutput StdErr;
extern KernelOutput StdDbg;

#endif /* _Output_h_ */
