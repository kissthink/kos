/******************************************************************************
    Copyright 2013 Martin Karsten

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
#include "util/Debug.h"
#include "util/Log.h"
#include "mach/Processor.h"
#include "kern/Kernel.h"
#include "kern/Thread.h"

extern "C" void abort() {
  kcout << "ABORT - ";
  Reboot();
}

// for C-style 'assert' (e.g., from malloc.c)
extern "C" void __assert_func( const char* const file, const char* const line,
  const char* const func, const char* const expr ) {
  kcerr << file << ':' << line << ' ' << func << ' ' << expr << kendl;
  kcdbg << file << ':' << line << ' ' << func << ' ' << expr << kendl;
}

#if 1
extern "C" void free(void* p) {
  DBG::outln(DBG::Libc, "LIBC/free: ", FmtHex(p));
  kernelVM.free(p);
}

extern "C" void* malloc(size_t s) {
  DBG::out(DBG::Libc, "LIBC/malloc: ", FmtHex(s));
  void* p = kernelVM.malloc(s);
  DBG::outln(DBG::Libc, " -> ", p);
  return p;
}
#endif

/******* dummy functions *******/

int __errno = 0;

extern "C" int close(int) {
  KASSERT(false, "close"); return -1;
}

extern "C" void _exit(int) {
  KASSERT(false, "_exit");
}

extern "C" int fstat(int fd, struct stat *buf) {
  KASSERT(false, "fstat"); return -1;
}

extern "C" char *getenv(const char *name) {
  DBG::outln(DBG::Libc, "LIBC/getenv: ", name);
  return nullptr;
}

extern "C" pid_t getpid() {
  KASSERT(false, "getpid"); return -1;
}

extern "C" int isatty(int fd) {
  KASSERT(false, "isatty"); return -1;
}

extern "C" int kill(pid_t pid, int sig) {
  KASSERT(false, "kill"); return -1;
}

extern "C" off_t lseek(int fd, off_t offset, int whence) {
  KASSERT(false, "lseek"); return -1;
}
  
extern "C" int open(const char *path, int oflag, int mode) {
  KASSERT(false, "close"); return -1;
}

extern "C" ssize_t read(int fd, char* buf, int len) {
  KASSERT(false, "read"); return -1;
}
  
extern "C" void* sbrk(intptr_t increment) {
  KASSERT(false, "sbrk"); return nullptr;
}

extern "C" ssize_t write(int fd, char* buf, int len) {
  KASSERT(false, "write"); return -1;
}

void *__dso_handle = nullptr;
