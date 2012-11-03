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
#ifndef _Log_h_
#define _Log_h_ 1

#include "util/basics.h"
#include "util/Bitmask.h"
#include "kern/globals.h"

#include <ostream>
#include <iomanip>
#include <cstdarg>

#undef __STRICT_ANSI__ // to get access to vsnprintf
#include <cstdio>

using namespace std;

extern ostream kcout;
extern ostream kcerr;
extern ostream kcdbg;
static const char kendl = '\n';

struct Printf {
  const char *format;
  va_list& args;
  Printf(const char *f, va_list& a) : format(f), args(a) {}
};

extern inline ostream& operator<<(ostream &os, const Printf& pf) {
  char* buffer = nullptr;
  int size = vsnprintf(buffer, 0, pf.format, pf.args);
  if (size >- 1) {
    size += 1; 
    buffer = new char[size];
    vsnprintf(buffer, size, pf.format, pf.args);
    os << buffer;
    globaldelete(buffer, size);
  }
  return os;
}

struct FmtPtr {
  uintptr_t ptr;
  int digits;
  FmtPtr(uintptr_t p, int d) : ptr(p), digits(d) {}
};

extern inline ostream& operator<<(ostream &os, const FmtPtr& fp) {
  os << setw(fp.digits) << (ptr_t)fp.ptr;
  return os;
}

// use external to avoid excessive inlining -> assembly code more readable
extern void kassertprint(const char* const loc, int line, const char* const func);

#if defined(KASSERT)
#error macro collision: KASSERT
#endif
#define KASSERT(expr,msg) { if unlikely(!(expr)) { kassertprint( "KASSERT: " #expr " in " __FILE__ ":", __LINE__, __func__); kcerr << msg; kcdbg << msg; Reboot(); } }

#endif /* _Log_h_ */
