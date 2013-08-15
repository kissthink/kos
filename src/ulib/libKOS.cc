#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

#if defined(__clang__)
typedef __ssize_t ssize_t;
#endif

#if !defined(__clang__)
extern "C" void abort() {
  for(;;) {}
}
#endif

extern "C" void* malloc(size_t s) {
  return nullptr;
}

extern "C" void free(void* p) {}

//extern "C" void* calloc(size_t nmemb, size_t size) {
//  return nullptr;
//}

extern "C" void* realloc(void *p, size_t size) {
  return nullptr;
}

extern "C" void* _malloc_r(void* r, size_t s) {
  return nullptr;
}

extern "C" void _free_r(void* r, void* p) {}

extern "C" void* _calloc_r(void* r, size_t n, size_t s) {
  return nullptr;
}

extern "C" void* _realloc_r(void* r, void *p, size_t size) {
  return nullptr;
}

extern "C" int close(int fd) {
  errno = EBADF;
  return -1;
}

extern "C" void _exit(int) {
  asm volatile("movq %0, %%rdi"::"i"(3) : "rdi", "memory");
  asm volatile("syscall" ::: "memory");
  for (;;);
}

#if !defined(__clang__)
extern "C" int fstat(int fd, struct stat *buf) {
  buf->st_mode = S_IFCHR;
  return 0;
}
#endif

extern "C" char *getenv(const char *name) { return nullptr; }

extern "C" int isatty(int fd) { return 1; }

#if defined(__clang__)
extern "C" ssize_t lseek(int fd, ssize_t offset, int whence) { return 0; }
#else
extern "C" off_t lseek(int fd, off_t offset, int whence) { return 0; }
#endif

extern "C" int open(const char *path, int oflag, int mode) {
  errno = ENOSYS;
  return -1;
}

extern "C" ssize_t read(int fd, void* buf, size_t len) { return 0; }

extern "C" void* sbrk(ptrdiff_t incr) {
  errno = ENOMEM;
  return (void*)-1;
}

extern "C" ssize_t write(int fd, const void* buf, size_t len) {
  if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
    // TODO: output
    return 0;
  }
  errno = EBADF;
  return -1;
}
