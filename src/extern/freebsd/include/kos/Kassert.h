#ifndef _KOS_KASSERT_h_
#define _KOS_KASSERT_h_

#include "kos/Basic.h"

// from KOS
EXTERNC void KOS_Reboot();
EXTERNC void KOS_KassertPrint(const char* const loc, int line, const char* const func);
EXTERNC void KOS_KassertPrint1(const char* const msg);
EXTERNC void KOS_KassertPrint2(const unsigned long long num);

#define BSD_KASSERT0(expr)        { if (!expr) { KOS_KassertPrint( "KASSERT: " #expr " in " __FILE__ ":", __LINE__, __func__); KOS_Reboot(); } }
#define BSD_KASSERTSTR(expr,msg)  { if (!expr) { KOS_KassertPrint( "KASSERT: " #expr " in " __FILE__ ":", __LINE__, __func__); KOS_KassertPrint1(msg); KOS_Reboot(); } }

#endif /* _KOS_KASSERT_h_ */
