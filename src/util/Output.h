#ifndef _iounlocked_h_
#define _iounlocked_h_ 1

#include "util/basics.h"
#include "mach/platform.h"

#include <ostream>
#include <iomanip>
#include <cstdarg>

struct FmtHex {
  ptr_t ptr;
  int digits;
  FmtHex(uintptr_t p,   int d = 0) : ptr((ptr_t)p), digits(d) {}
  FmtHex(ptr_t p,       int d = 0) : ptr((ptr_t)p), digits(d) {}
  FmtHex(const char* p, int d = 0) : ptr((ptr_t)p), digits(d) {}
};

inline ostream& operator<<(ostream &os, const FmtHex& hex) {
  os << setw(hex.digits) << hex.ptr;
  return os;
}

static const char kendl = '\n';

// defined in Output.cc
extern void kassertprint(const char* const loc, int line, const char* const func);
extern void kassertprint1(const char* const msg);
extern void kassertprint1(const unsigned long long num);
extern void kassertprint1(const FmtHex& ptr);

#if defined(ABORT0)
#error macro collision: ABORT0
#endif
#if defined(ABORT1)
#error macro collision: ABORT1
#endif
#if defined(KASSERT0)
#error macro collision: KASSERT0
#endif
#if defined(KASSERT1)
#error macro collision: KASSERT1
#endif

#define ABORT0()           {                      { kassertprint( "ABORT  : "       " in " __FILE__ ":", __LINE__, __func__); Reboot(); } }
#define ABORT1(msg)        {                      { kassertprint( "ABORT  : "       " in " __FILE__ ":", __LINE__, __func__); kassertprint1(msg); Reboot(); } }
#define KASSERT0(expr)     { if unlikely(!(expr)) { kassertprint( "KASSERT: " #expr " in " __FILE__ ":", __LINE__, __func__); Reboot(); } }
#define KASSERT1(expr,msg) { if unlikely(!(expr)) { kassertprint( "KASSERT: " #expr " in " __FILE__ ":", __LINE__, __func__); kassertprint1(msg); Reboot(); } }

#endif /* _iounlocked_h_ */
