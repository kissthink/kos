/*-
 * Copyright (c) 1987, 1991, 1993
 *	The Regents of the University of California.
 * Copyright (c) 2005-2009 Robert N. M. Watson
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)kern_malloc.c	8.3 (Berkeley) 1/4/94
 */

/*
 * Kernel malloc(9) implementation -- general purpose kernel memory allocator
 * based on memory types.  Back end is implemented using the UMA(9) zone
 * allocator.  A set of fixed-size buckets are used for smaller allocations,
 * and a special UMA allocation interface is used for larger allocations.
 * Callers declare memory types, and statistics are maintained independently
 * for each memory type.  Statistics are maintained per-CPU for performance
 * reasons.  See malloc(9) and comments in malloc.h for a detailed
 * description.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: release/9.1.0/sys/kern/kern_malloc.c 230418 2012-01-21 05:03:10Z alc $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mutex.h>
#include <sys/vmmeter.h>
#include <sys/sbuf.h>
#include <sys/time.h>

#include <vm/vm.h>
#include <vm/pmap.h>

#include "kos/Kassert.h"
#include "kos/Malloc.h"
#include "kos/Sched.h"
#include "kos/Timer.h"

/*
 * When realloc() is called, if the new size is sufficiently smaller than
 * the old size, realloc() will allocate a new, smaller block to avoid
 * wasting memory. 'Sufficiently smaller' is defined as: newsize <=
 * oldsize / 2^n, where REALLOC_FRACTION defines the value of 'n'.
 */
#ifndef REALLOC_FRACTION
#define	REALLOC_FRACTION	1	/* new block if <= half the size */
#endif

/*
 * Centrally define some common malloc types.
 */
MALLOC_DEFINE(M_CACHE, "cache", "Various Dynamically allocated caches");
MALLOC_DEFINE(M_DEVBUF, "devbuf", "device driver memory");
MALLOC_DEFINE(M_TEMP, "temp", "misc temporary data buffers");

MALLOC_DEFINE(M_IP6OPT, "ip6opt", "IPv6 options");
MALLOC_DEFINE(M_IP6NDP, "ip6ndp", "IPv6 Neighbor Discovery");

static MALLOC_DEFINE(M_FREE, "free", "should be on free list");

/*
 * The malloc_mtx protects the kmemstatistics linked list.
 */
struct mtx malloc_mtx;

/*
 * time_uptime of the last malloc(9) failure (induced or real).
 */
static time_t t_malloc_fail;

int
malloc_last_fail(void)
{
  return KOS_GetUpTime() - t_malloc_fail;
}

/*
 * An allocation has succeeded -- update malloc type statistics for the
 * amount of bucket size.  Occurs within a critical section so that the
 * thread isn't preempted and doesn't migrate while updating per-PCU
 * statistics.
 */
void
malloc_type_allocated(struct malloc_type *mtp, unsigned long size)
{

}

/*
 * A free operation has occurred -- update malloc type statistics for the
 * amount of the bucket size.  Occurs within a critical section so that the
 * thread isn't preempted and doesn't migrate while updating per-CPU
 * statistics.
 */
void
malloc_type_freed(struct malloc_type *mtp, unsigned long size)
{

}

/*
 *	malloc:
 *
 *	Allocate a block of memory.
 *
 *	If M_NOWAIT is set, this routine will not block and return NULL if
 *	the allocation fails.
 */
void *
malloc(unsigned long size, struct malloc_type *mtp, int flags)
{
	caddr_t va;

	if (flags & M_WAITOK)
    BSD_KASSERTSTR((!KOS_InInterrupt()), "malloc(M_WAITOK) in interrupt context");

  va = KOS_Malloc(size);
  if (va != NULL && flags & M_ZERO) {
    bzero(va, size);
  }
	if (flags & M_WAITOK) {
    BSD_KASSERTSTR((va != NULL), "malloc(M_WAITOK) returned NULL");
  } else if (va == NULL) {
    t_malloc_fail = KOS_GetUpTime();
  }
	return ((void *) va);
}

/*
 *	free:
 *
 *	Free a block of memory allocated by malloc.
 *
 *	This routine may not block.
 */
void
free(void *addr, struct malloc_type *mtp)
{
	u_long size = 0;

	/* free(NULL, ...) does nothing */
	if (addr == NULL)
		return;

  KOS_Free(addr);
	malloc_type_freed(mtp, size);
}

/*
 *	realloc: change the size of a memory block
 */
void *
realloc(void *addr, unsigned long size, struct malloc_type *mtp, int flags)
{
	unsigned long alloc;
	void *newaddr;

	/* realloc(NULL, ...) is equivalent to malloc(...) */
	if (addr == NULL)
		return (malloc(size, mtp, flags));

  /* Get the size of the original block */
  alloc = KOS_GetMemorySize(addr);

  /* Reuse the original block if appropriate */
  if (size <= alloc) return addr;

	/* Allocate a new, bigger (or smaller) block */
	if ((newaddr = malloc(size, mtp, flags)) == NULL)
		return (NULL);

	/* Copy over original contents */
	bcopy(addr, newaddr, min(size, alloc));
	free(addr, mtp);
	return (newaddr);
}

/*
 *	reallocf: same as realloc() but free memory on failure.
 */
void *
reallocf(void *addr, unsigned long size, struct malloc_type *mtp, int flags)
{
	void *mem;

	if ((mem = realloc(addr, size, mtp, flags)) == NULL)
		free(addr, mtp);
	return (mem);
}

void
malloc_init(void *data)
{

}

void
malloc_uninit(void *data)
{

}

struct malloc_type *
malloc_desc2type(const char *desc)
{
  // sys/vm/memguard.c
	return (NULL);
}

void
malloc_type_list(malloc_type_list_func_t *func, void *arg)
{
  // sys/cddl/dev/dtmalloc/dtmalloc.c
}
