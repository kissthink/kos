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
#ifndef _Log_h_
#define _Log_h_ 1

#include "util/basics.h"
#include "kern/globals.h"

#include <ostream>
#include <iomanip>
#include <cstdarg>

#undef __STRICT_ANSI__ // to get access to vsnprintf
#include <cstdio>

extern std::ostream kcout;
extern std::ostream kcerr;
extern std::ostream kcdbg;
static const char kendl = '\n';

struct Printf {
  const char *format;
  va_list& args;
  Printf(const char *f, va_list& a) : format(f), args(a) {}
};

extern inline std::ostream& operator<<(std::ostream &os, const Printf& pf) {
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

struct FmtHex {
  ptr_t ptr;
  int digits;
  FmtHex(uintptr_t p, int d = 0) : ptr((ptr_t)p), digits(d) {}
  FmtHex(ptr_t p, int d = 0) : ptr((ptr_t)p), digits(d) {}
  FmtHex(const char* p, int d = 0) : ptr((ptr_t)p), digits(d) {}
};

extern inline std::ostream& operator<<(std::ostream &os, const FmtHex& hex) {
  os << std::setw(hex.digits) << hex.ptr;
  return os;
}

// use external to avoid excessive inlining -> assembly code more readable
extern void kassertprint(const char* const loc, int line, const char* const func);

#if defined(KASSERT)
#error macro collision: KASSERT
#endif
#define KASSERT(expr,msg) { if unlikely(!(expr)) { kassertprint( "KASSERT: " #expr " in " __FILE__ ":", __LINE__, __func__); kcerr << msg; kcdbg << msg; Reboot(); } }

#endif /* _Log_h_ */
