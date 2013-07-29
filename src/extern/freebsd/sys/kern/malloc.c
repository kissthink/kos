#include "sys/types.h"
#include "sys/malloc.h"
#include "sys/systm.h"
#include "kos/kos_kassert.h"
#include "kos/kos_malloc.h"

/**
 * Possible flags
 * M_ZERO - zero-out the allocated memory
 * M_NOWAIT - malloc returns NULL if allocation cannot be done immediately
 * M_WAITOK - current process is put to sleep if allocation cannot be done
 *            immediately. (malloc cannot return NULL)
 */
void *
malloc(unsigned long size, struct malloc_type *mtp, int flags) {
  // Check that exactly one of M_WAITOK or M_NOWAIT is specified.
  int checkFlags = flags & (M_WAITOK | M_NOWAIT);
  if (checkFlags != M_WAITOK && checkFlags != M_NOWAIT) {
    printf("Bad malloc flags: %x\n", flags);
    return NULL;
  }

  // Check if in interrupt context if M_WAITOK is set
  if (flags & M_WAITOK) {
    BSD_KASSERTSTR(1 /* if in interrupt context */,
      "malloc(M_WAITOK) in interrupt context");
  }

  // TODO should support M_WAITOK in KOS
  void* mem =  kos_malloc(size);
  if (mem && (flags & M_ZERO)) bzero(mem, size);    // zero-out for M_ZERO
  return mem;
}

/**
 * May not block.
 */
void
free(void *addr, struct malloc_type *mtp) {
	/* free(NULL, ...) does nothing */
	if (addr == NULL)
		return;
  
  kos_free(addr);
}

/*
 *	realloc: change the size of a memory block
 */
void *
realloc(void *addr, unsigned long size, struct malloc_type *mtp, int flags) {
	unsigned long alloc;    // get original block's size
	void *newaddr;

	// realloc(NULL, ...) is equivalent to malloc(...)
	if (addr == NULL)
		return (malloc(size, mtp, flags));

  // get original block size
  alloc = kos_getMemorySize(addr);
  if (!alloc) return NULL;

  // allocate a new, bigger (or smaller) lock
  if ((newaddr = malloc(size, mtp, flags)) == NULL)
    return NULL;

	// Copy over original contents
	bcopy(addr, newaddr, min(size, alloc));
	free(addr, mtp);

  // zero-out for M_ZERO
  if (flags & M_ZERO) bzero(newaddr, size);
	return newaddr;
}

/*
 *	reallocf: same as realloc() but free memory on failure.
 */
void *
reallocf(void *addr, unsigned long size, struct malloc_type *mtp, int flags) {
	void *mem;

	if ((mem = realloc(addr, size, mtp, flags)) == NULL)
		free(addr, mtp);
	return mem;
}

