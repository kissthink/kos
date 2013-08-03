#include "sys/types.h"
#include "sys/malloc.h"
#include "sys/systm.h"
#include "kos/Kassert.h"
#include "kos/Malloc.h"
#include "kos/Sched.h"

/*
 * malloc:
 *
 * Allocate a block of memory.
 *
 * If M_NOWAIT is set, this routine will not block and return NULL if
 * the allocation fails.
 *
 * Possible flags
 * M_ZERO   - zero-out the allocated memory
 * M_NOWAIT - NULL if allocation cannot be done immediately
 * M_WAITOK - current process is put to sleep if allocation cannot be done
 *            immediately. (malloc cannot return NULL)
 */
void *malloc(unsigned long size, struct malloc_type *mtp, int flags) {
  void *mem = NULL;

  // check exactly one of M_WAITOK or M_NOWAIT is specified
  int checkFlags = flags & (M_WAITOK | M_NOWAIT);
  if (checkFlags != M_WAITOK && checkFlags != M_NOWAIT) {
    printf("Bad malloc flags: %x\n", flags);
    return mem;
  }

  // M_WAITOK flag is not valid in interrupt context (cannot sleep)
  if (flags & M_WAITOK)
    BSD_KASSERTSTR(!KOS_InInterrupt(), "malloc(M_WAITOK) in interrupt context");

  // TODO should support M_WAITOK in KOS
  mem = KOS_Malloc(size);
  if (mem && (flags & M_ZERO))
    bzero(mem, size);    // zero-out for M_ZERO
  return mem;
}

/*
 * free:
 *
 * Free a block of memory allocated by malloc.
 *
 * This routine may not block.
 */
void free(void *addr, struct malloc_type *mtp) {
	if (addr == NULL)
		return;
  
  KOS_Free(addr);
}

/*
 *	realloc: change the size of a memory block
 */
void *realloc(void *addr, unsigned long size, struct malloc_type *mtp, int flags) {
	unsigned long alloc;    // get original block's size
	void *newaddr;

	// realloc(NULL, ...) is equivalent to malloc(...)
	if (addr == NULL)
		return (malloc(size, mtp, flags));

  // get original block size
  alloc = KOS_GetMemorySize(addr);
  if (size <= alloc) return addr;

  // allocate a new, bigger (or smaller) lock
  if ((newaddr = malloc(size, mtp, flags)) == NULL)
    return NULL;

	// Copy over original contents
	bcopy(addr, newaddr, min(size, alloc));
	free(addr, mtp);

	return newaddr;
}

/*
 *	reallocf: same as realloc() but free memory on failure.
 */
void *reallocf(void *addr, unsigned long size, struct malloc_type *mtp, int flags) {
	void *mem;

	if ((mem = realloc(addr, size, mtp, flags)) == NULL)
		free(addr, mtp);
	return mem;
}
