#include "kos/kos_frameManager.h"
#include "sys/malloc.h"

// TODO do not ignore flags, alignment, boundary
void *
contigmalloc(
	unsigned long size,	/* should be size_t here and for malloc() */
	struct malloc_type *type,
	int flags,
	vm_paddr_t low,
	vm_paddr_t high,
	unsigned long alignment,
	unsigned long boundary) {
	void *ret;
  ret = kos_alloc_contig(size, low, high, alignment, boundary);
	return ret;
}

void
contigfree(void *addr, unsigned long size, struct malloc_type *type) {
  kos_free_contig(addr, size);
}
