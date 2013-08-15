/*-
 * Copyright (c) 2006 John Baldwin <jhb@FreeBSD.org>
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
 * 3. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * This module holds the global variables and functions used to maintain
 * lock_object structures.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: release/9.1.0/sys/kern/subr_lock.c 235404 2012-05-13 17:01:32Z avg $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/mutex.h>
#include <sys/pcpu.h>
#include <sys/proc.h>
#include <sys/sbuf.h>
#include <sys/sched.h>
#include <sys/smp.h>

#include <machine/cpufunc.h>

CTASSERT(LOCK_CLASS_MAX == 15);

struct lock_class *lock_classes[LOCK_CLASS_MAX + 1] = {
	&lock_class_mtx_spin,
	&lock_class_mtx_sleep,
	&lock_class_sx,
	&lock_class_rm,
	&lock_class_rw,
//	&lock_class_lockmgr,
};

void
lock_init(struct lock_object *lock, struct lock_class *class, const char *name,
    const char *type, int flags)
{
	int i;

	/* Check for double-init and zero object. */
	KASSERT(!lock_initalized(lock), ("lock \"%s\" %p already initialized",
	    name, lock));

	/* Look up lock class to find its index. */
	for (i = 0; i < LOCK_CLASS_MAX; i++)
		if (lock_classes[i] == class) {
			lock->lo_flags = i << LO_CLASSSHIFT;
			break;
		}
	KASSERT(i < LOCK_CLASS_MAX, ("unknown lock class %p", class));

	/* Initialize the lock object. */
	lock->lo_name = name;
	lock->lo_flags |= flags | LO_INITIALIZED;
}

void
lock_destroy(struct lock_object *lock)
{

	KASSERT(lock_initalized(lock), ("lock %p is not initialized", lock));
	lock->lo_flags &= ~LO_INITIALIZED;
}
