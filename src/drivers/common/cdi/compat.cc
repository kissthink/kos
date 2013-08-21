#include "util/basics.h"
#include "kern/Debug.h"
#include <cstdio>

int printf(const char* fmt, ...)
{
    static char print_temp[1024];
    va_list argptr;
    va_start(argptr, fmt);
    int ret = vsprintf(print_temp, fmt, argptr);
    NOTICE(print_temp);
    va_end(argptr);
    return ret;
}

int snprintf(char *s, size_t n, const char *fmt, ...)
{
    va_list argptr;
    va_start(argptr, fmt);
    int ret = vsprintf(s, fmt, argptr);
    va_end(argptr);
    return ret;
}

int sprintf(char *s, const char *fmt, ...)
{
    va_list argptr;
    va_start(argptr, fmt);
    int ret = vsprintf(s, fmt, argptr);
    va_end(argptr);
    return ret;
}

int puts(const char *s)
{
    NOTICE(s);
    return strlen(s);
}
