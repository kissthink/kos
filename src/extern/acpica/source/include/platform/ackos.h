#ifndef __ACKOS_H__
#define __ACKOS_H__

/*
 * ACPICA configuration
 */
//#define ACPI_USE_SYSTEM_CLIBRARY
//#define ACPI_FLUSH_CPU_CACHE()

#define ACPI_USE_DO_WHILE_0
#define ACPI_USE_LOCAL_CACHE

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

#if defined(__ia64__) || defined(__x86_64__)
#define ACPI_MACHINE_WIDTH          64
#define COMPILER_DEPENDENT_INT64    long
#define COMPILER_DEPENDENT_UINT64   unsigned long
#else
#error unsupported architecture: only __x86_64__ supported at this time
#endif

#ifndef __cdecl
#define __cdecl
#endif

#include "acgcc.h"

#endif /* __ACKOS_H__ */
