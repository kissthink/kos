#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

#if defined(__clang__)
typedef __ssize_t ssize_t;
#endif

extern "C" void abort() {
  for(;;) {}
}

extern "C" int close(int fd) {
  errno = EBADF;
  return -1;
}

#if !defined(__clang__)
extern "C" int fstat(int fd, struct stat *buf) {
  buf->st_mode = S_IFCHR;
  return 0;
}
#endif

extern "C" char *getenv(const char *name) {
  return nullptr;
}

extern "C" int isatty(int fd) {
  return 1;
}

extern "C" off_t lseek(int fd, off_t offset, int whence) {
  return 0;
}

extern "C" int open(const char *path, int oflag, int mode) {
  errno = ENOSYS;
  return -1;
}

extern "C" int read(int fd, void* buf, size_t len) {
  return 0;
}

extern "C" void* sbrk(ptrdiff_t incr) {
  errno = ENOMEM;
  return (void*)-1;
}

extern "C" int write(int fd, const void* buf, size_t len) {
  if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
    // TODO: output
    return 0;
  }
  errno = EBADF;
  return -1;
}
