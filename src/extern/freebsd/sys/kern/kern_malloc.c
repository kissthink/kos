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

#ifdef SUKWON
#include "opt_ddb.h"
#include "opt_kdtrace.h"
#include "opt_vm.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kdb.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mutex.h>
#include <sys/vmmeter.h>
#include <sys/proc.h>
#include <sys/sbuf.h>
#include <sys/sysctl.h>
#include <sys/time.h>

#include <vm/vm.h>
#include <vm/pmap.h>
#ifdef SUKWON
#include <vm/vm_param.h>
#include <vm/vm_kern.h>
#include <vm/vm_extern.h>
#include <vm/vm_map.h>
#include <vm/vm_page.h>
#include <vm/uma.h>
#include <vm/uma_int.h>
#include <vm/uma_dbg.h>
#endif

#if defined(INVARIANTS) && defined(__i386__)
#include <machine/cpu.h>
#endif

#ifdef SUKWON
#include <ddb/ddb.h>
#endif

#ifndef SUKWON
#include "kos/Kassert.h"
#include "kos/Malloc.h"
#include "kos/Sched.h"
#endif

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

#ifdef SUKWON
static void kmeminit(void *);
SYSINIT(kmem, SI_SUB_KMEM, SI_ORDER_FIRST, kmeminit, NULL);
#endif

static MALLOC_DEFINE(M_FREE, "free", "should be on free list");

#ifdef SUKWON
static struct malloc_type *kmemstatistics;
static vm_offset_t kmembase;
static vm_offset_t kmemlimit;
static int kmemcount;

#define KMEM_ZSHIFT	4
#define KMEM_ZBASE	16
#define KMEM_ZMASK	(KMEM_ZBASE - 1)

#define KMEM_ZMAX	PAGE_SIZE
#define KMEM_ZSIZE	(KMEM_ZMAX >> KMEM_ZSHIFT)
static uint8_t kmemsize[KMEM_ZSIZE + 1];

#ifndef MALLOC_DEBUG_MAXZONES
#define	MALLOC_DEBUG_MAXZONES	1
#endif
static int numzones = MALLOC_DEBUG_MAXZONES;

/*
 * Small malloc(9) memory allocations are allocated from a set of UMA buckets
 * of various sizes.
 *
 * XXX: The comment here used to read "These won't be powers of two for
 * long."  It's possible that a significant amount of wasted memory could be
 * recovered by tuning the sizes of these buckets.
 */
struct {
	int kz_size;
	char *kz_name;
	uma_zone_t kz_zone[MALLOC_DEBUG_MAXZONES];
} kmemzones[] = {
	{16, "16", },
	{32, "32", },
	{64, "64", },
	{128, "128", },
	{256, "256", },
	{512, "512", },
	{1024, "1024", },
	{2048, "2048", },
	{4096, "4096", },
#if PAGE_SIZE > 4096
	{8192, "8192", },
#if PAGE_SIZE > 8192
	{16384, "16384", },
#if PAGE_SIZE > 16384
	{32768, "32768", },
#if PAGE_SIZE > 32768
	{65536, "65536", },
#if PAGE_SIZE > 65536
#error	"Unsupported PAGE_SIZE"
#endif	/* 65536 */
#endif	/* 32768 */
#endif	/* 16384 */
#endif	/* 8192 */
#endif	/* 4096 */
	{0, NULL},
};

/*
 * Zone to allocate malloc type descriptions from.  For ABI reasons, memory
 * types are described by a data structure passed by the declaring code, but
 * the malloc(9) implementation has its own data structure describing the
 * type and statistics.  This permits the malloc(9)-internal data structures
 * to be modified without breaking binary-compiled kernel modules that
 * declare malloc types.
 */
static uma_zone_t mt_zone;

u_long vm_kmem_size;
SYSCTL_ULONG(_vm, OID_AUTO, kmem_size, CTLFLAG_RDTUN, &vm_kmem_size, 0,
    "Size of kernel memory");

static u_long vm_kmem_size_min;
SYSCTL_ULONG(_vm, OID_AUTO, kmem_size_min, CTLFLAG_RDTUN, &vm_kmem_size_min, 0,
    "Minimum size of kernel memory");

static u_long vm_kmem_size_max;
SYSCTL_ULONG(_vm, OID_AUTO, kmem_size_max, CTLFLAG_RDTUN, &vm_kmem_size_max, 0,
    "Maximum size of kernel memory");

static u_int vm_kmem_size_scale;
SYSCTL_UINT(_vm, OID_AUTO, kmem_size_scale, CTLFLAG_RDTUN, &vm_kmem_size_scale, 0,
    "Scale factor for kernel memory size");

static int sysctl_kmem_map_size(SYSCTL_HANDLER_ARGS);
SYSCTL_PROC(_vm, OID_AUTO, kmem_map_size,
    CTLFLAG_RD | CTLTYPE_ULONG | CTLFLAG_MPSAFE, NULL, 0,
    sysctl_kmem_map_size, "LU", "Current kmem_map allocation size");

static int sysctl_kmem_map_free(SYSCTL_HANDLER_ARGS);
SYSCTL_PROC(_vm, OID_AUTO, kmem_map_free,
    CTLFLAG_RD | CTLTYPE_ULONG | CTLFLAG_MPSAFE, NULL, 0,
    sysctl_kmem_map_free, "LU", "Largest contiguous free range in kmem_map");
#endif

/*
 * The malloc_mtx protects the kmemstatistics linked list.
 */
struct mtx malloc_mtx;

#ifdef SUKWON
static int sysctl_kern_malloc_stats(SYSCTL_HANDLER_ARGS);
#endif

/*
 * time_uptime of the last malloc(9) failure (induced or real).
 */
static time_t t_malloc_fail;

#ifdef SUKWON
static int
sysctl_kmem_map_size(SYSCTL_HANDLER_ARGS)
{
	u_long size;

	size = kmem_map->size;
	return (sysctl_handle_long(oidp, &size, 0, req));
}

static int
sysctl_kmem_map_free(SYSCTL_HANDLER_ARGS)
{
	u_long size;

	vm_map_lock_read(kmem_map);
	size = kmem_map->root != NULL ? kmem_map->root->max_free :
	    kmem_map->max_offset - kmem_map->min_offset;
	vm_map_unlock_read(kmem_map);
	return (sysctl_handle_long(oidp, &size, 0, req));
}

/*
 * malloc(9) uma zone separation -- sub-page buffer overruns in one
 * malloc type will affect only a subset of other malloc types.
 */
#if MALLOC_DEBUG_MAXZONES > 1
static void
tunable_set_numzones(void)
{

	TUNABLE_INT_FETCH("debug.malloc.numzones",
	    &numzones);

	/* Sanity check the number of malloc uma zones. */
	if (numzones <= 0)
		numzones = 1;
	if (numzones > MALLOC_DEBUG_MAXZONES)
		numzones = MALLOC_DEBUG_MAXZONES;
}
SYSINIT(numzones, SI_SUB_TUNABLES, SI_ORDER_ANY, tunable_set_numzones, NULL);
SYSCTL_INT(_debug_malloc, OID_AUTO, numzones, CTLFLAG_RDTUN,
    &numzones, 0, "Number of malloc uma subzones");

/*
 * Any number that changes regularly is an okay choice for the
 * offset.  Build numbers are pretty good of you have them.
 */
static u_int zone_offset = __FreeBSD_version;
TUNABLE_INT("debug.malloc.zone_offset", &zone_offset);
SYSCTL_UINT(_debug_malloc, OID_AUTO, zone_offset, CTLFLAG_RDTUN,
    &zone_offset, 0, "Separate malloc types by examining the "
    "Nth character in the malloc type short description.");

static u_int
mtp_get_subzone(const char *desc)
{
	size_t len;
	u_int val;

	if (desc == NULL || (len = strlen(desc)) == 0)
		return (0);
	val = desc[zone_offset % len];
	return (val % numzones);
}
#elif MALLOC_DEBUG_MAXZONES == 0
#error "MALLOC_DEBUG_MAXZONES must be positive."
#else
static inline u_int
mtp_get_subzone(const char *desc)
{

	return (0);
}
#endif /* MALLOC_DEBUG_MAXZONES > 1 */
#endif

int
malloc_last_fail(void)
{
#ifdef SUKWON
	return (time_uptime - t_malloc_fail);
#else
  return KOS_GetUpTime() - t_malloc_fail;
#endif
}

/*
 * An allocation has succeeded -- update malloc type statistics for the
 * amount of bucket size.  Occurs within a critical section so that the
 * thread isn't preempted and doesn't migrate while updating per-PCU
 * statistics.
 */
static void
malloc_type_zone_allocated(struct malloc_type *mtp, unsigned long size,
    int zindx)
{
#ifdef SUKWON
	struct malloc_type_internal *mtip;
	struct malloc_type_stats *mtsp;

	critical_enter();
	mtip = mtp->ks_handle;
	mtsp = &mtip->mti_stats[curcpu];
	if (size > 0) {
		mtsp->mts_memalloced += size;
		mtsp->mts_numallocs++;
	}
	if (zindx != -1)
		mtsp->mts_size |= 1 << zindx;

	critical_exit();
#endif
}

void
malloc_type_allocated(struct malloc_type *mtp, unsigned long size)
{
#ifdef SUKWON // sys/vm/vm_contig.c; sys/compat/ndis/subr_ntoskrnl.c
	if (size > 0)
		malloc_type_zone_allocated(mtp, size, -1);
#endif
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
#ifdef SUKWON // sys/vm/vm_contig.c
	struct malloc_type_internal *mtip;
	struct malloc_type_stats *mtsp;

	critical_enter();
	mtip = mtp->ks_handle;
	mtsp = &mtip->mti_stats[curcpu];
	mtsp->mts_memfreed += size;
	mtsp->mts_numfrees++;

	critical_exit();
#endif
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
#ifdef SUKWON
	uma_slab_t slab;
#endif
	u_long size;

#ifdef SUKWON
	KASSERT(mtp->ks_magic == M_MAGIC, ("free: bad malloc type magic"));
#endif

	/* free(NULL, ...) does nothing */
	if (addr == NULL)
		return;

#ifdef SUKWON
	slab = vtoslab((vm_offset_t)addr & (~UMA_SLAB_MASK));

	if (slab == NULL)
		panic("free: address %p(%p) has not been allocated.\n",
		    addr, (void *)((u_long)addr & (~UMA_SLAB_MASK)));


	if (!(slab->us_flags & UMA_SLAB_MALLOC)) {
		size = slab->us_keg->uk_size;
		uma_zfree_arg(LIST_FIRST(&slab->us_keg->uk_zones), addr, slab);
	} else {
		size = slab->us_size;
		uma_large_free(slab);
	}
#else
  KOS_Free(addr);
#endif
	malloc_type_freed(mtp, size);
}

/*
 *	realloc: change the size of a memory block
 */
void *
realloc(void *addr, unsigned long size, struct malloc_type *mtp, int flags)
{
#ifdef SUKWON
	uma_slab_t slab;
#endif
	unsigned long alloc;
	void *newaddr;

#ifdef SUKWON
	KASSERT(mtp->ks_magic == M_MAGIC,
	    ("realloc: bad malloc type magic"));
#endif

	/* realloc(NULL, ...) is equivalent to malloc(...) */
	if (addr == NULL)
		return (malloc(size, mtp, flags));

#ifdef SUKWON
	/*
	 * XXX: Should report free of old memory and alloc of new memory to
	 * per-CPU stats.
	 */

	slab = vtoslab((vm_offset_t)addr & ~(UMA_SLAB_MASK));

	/* Sanity check */
	KASSERT(slab != NULL,
	    ("realloc: address %p out of range", (void *)addr));

	/* Get the size of the original block */
	if (!(slab->us_flags & UMA_SLAB_MALLOC))
		alloc = slab->us_keg->uk_size;
	else
		alloc = slab->us_size;

	/* Reuse the original block if appropriate */
	if (size <= alloc
	    && (size > (alloc >> REALLOC_FRACTION) || alloc == MINALLOCSIZE))
		return (addr);
#else
  /* Get the size of the original block */
  alloc = KOS_GetMemorySize(addr);

  /* Reuse the original block if appropriate */
  if (size <= alloc) return addr;
#endif

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
#ifdef SUKWON
	struct malloc_type_internal *mtip;
	struct malloc_type *mtp;

	KASSERT(cnt.v_page_count != 0, ("malloc_register before vm_init"));

	mtp = data;
	if (mtp->ks_magic != M_MAGIC)
		panic("malloc_init: bad malloc type magic");

	mtip = uma_zalloc(mt_zone, M_WAITOK | M_ZERO);
	mtp->ks_handle = mtip;
	mtip->mti_zone = mtp_get_subzone(mtp->ks_shortdesc);

	mtx_lock(&malloc_mtx);
	mtp->ks_next = kmemstatistics;
	kmemstatistics = mtp;
	kmemcount++;
	mtx_unlock(&malloc_mtx);
#endif
}

void
malloc_uninit(void *data)
{
#ifdef SUKWON
	struct malloc_type_internal *mtip;
	struct malloc_type_stats *mtsp;
	struct malloc_type *mtp, *temp;
	uma_slab_t slab;
	long temp_allocs, temp_bytes;
	int i;

	mtp = data;
	KASSERT(mtp->ks_magic == M_MAGIC,
	    ("malloc_uninit: bad malloc type magic"));
	KASSERT(mtp->ks_handle != NULL, ("malloc_deregister: cookie NULL"));

	mtx_lock(&malloc_mtx);
	mtip = mtp->ks_handle;
	mtp->ks_handle = NULL;
	if (mtp != kmemstatistics) {
		for (temp = kmemstatistics; temp != NULL;
		    temp = temp->ks_next) {
			if (temp->ks_next == mtp) {
				temp->ks_next = mtp->ks_next;
				break;
			}
		}
		KASSERT(temp,
		    ("malloc_uninit: type '%s' not found", mtp->ks_shortdesc));
	} else
		kmemstatistics = mtp->ks_next;
	kmemcount--;
	mtx_unlock(&malloc_mtx);

	/*
	 * Look for memory leaks.
	 */
	temp_allocs = temp_bytes = 0;
	for (i = 0; i < MAXCPU; i++) {
		mtsp = &mtip->mti_stats[i];
		temp_allocs += mtsp->mts_numallocs;
		temp_allocs -= mtsp->mts_numfrees;
		temp_bytes += mtsp->mts_memalloced;
		temp_bytes -= mtsp->mts_memfreed;
	}
	if (temp_allocs > 0 || temp_bytes > 0) {
		printf("Warning: memory type %s leaked memory on destroy "
		    "(%ld allocations, %ld bytes leaked).\n", mtp->ks_shortdesc,
		    temp_allocs, temp_bytes);
	}

	slab = vtoslab((vm_offset_t) mtip & (~UMA_SLAB_MASK));
	uma_zfree_arg(mt_zone, mtip, slab);
#endif
}

struct malloc_type *
malloc_desc2type(const char *desc)
{
#ifdef SUKWON // sys/vm/memguard.c
	struct malloc_type *mtp;

	mtx_assert(&malloc_mtx, MA_OWNED);
	for (mtp = kmemstatistics; mtp != NULL; mtp = mtp->ks_next) {
		if (strcmp(mtp->ks_shortdesc, desc) == 0)
			return (mtp);
	}
#endif
	return (NULL);
}

void
malloc_type_list(malloc_type_list_func_t *func, void *arg)
{
#ifdef SUKWON   // sys/cddl/dev/dtmalloc/dtmalloc.c
	struct malloc_type *mtp, **bufmtp;
	int count, i;
	size_t buflen;

	mtx_lock(&malloc_mtx);
restart:
	mtx_assert(&malloc_mtx, MA_OWNED);
	count = kmemcount;
	mtx_unlock(&malloc_mtx);

	buflen = sizeof(struct malloc_type *) * count;
	bufmtp = malloc(buflen, M_TEMP, M_WAITOK);

	mtx_lock(&malloc_mtx);

	if (count < kmemcount) {
		free(bufmtp, M_TEMP);
		goto restart;
	}

	for (mtp = kmemstatistics, i = 0; mtp != NULL; mtp = mtp->ks_next, i++)
		bufmtp[i] = mtp;

	mtx_unlock(&malloc_mtx);

	for (i = 0; i < count; i++)
		(func)(bufmtp[i], arg);

	free(bufmtp, M_TEMP);
#endif
}
