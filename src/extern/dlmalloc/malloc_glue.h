#include <cstddef>
#include <sys/types.h>

// dlmalloc configurations

#define ONLY_MSPACES 1
#define NO_MALLOC_STATS 1
#define LACKS_SYS_MMAN_H
#define LACKS_TIME_H
#define ABORT_ON_ASSERT_FAILURE 0
#define MMAP_CLEARS 0
#define malloc_getpagesize 4096    // avoid call to 'sysconf'
#define DEFAULT_GRANULARITY 8192

// mman.h

#define MAP_ANONYMOUS 0            // avoids call to 'open'
#define PROT_READ 0
#define PROT_WRITE 0
#define MAP_PRIVATE 0

extern "C" void *mmap(void*, size_t, int, int, int, _off64_t);
extern "C" int munmap(void*, size_t);