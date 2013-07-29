#ifndef _KOS_KASSERT_h_
#define _KOS_KASSERT_h_

/**
 * useful macros
 */

#if defined(EXTERNC)
#error macro collision: EXTERNC
#endif

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

// from KOS
EXTERNC void kos_reboot();
EXTERNC void kos_kassertprint(const char* const loc, int line, const char* const func);
EXTERNC void kos_kassertprint1(const char* const msg);
EXTERNC void kos_kassertprint2(const unsigned long long num);

#define BSD_KASSERT0(expr)        { if (!expr) { kos_kassertprint( "KASSERT: " #expr " in " __FILE__ ":", __LINE__, __func__); kos_reboot(); } }
#define BSD_KASSERTSTR(expr,msg)  { if (!expr) { kos_kassertprint( "KASSERT: " #expr " in " __FILE__ ":", __LINE__, __func__); kos_kassertprint1(msg); kos_reboot(); } }

#undef EXTERNC

#endif /* _KOS_KASSERT_h_ */
