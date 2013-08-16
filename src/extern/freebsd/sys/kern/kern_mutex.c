/*-
 * Copyright (c) 1998 Berkeley Software Design, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Berkeley Software Design Inc's name may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BERKELEY SOFTWARE DESIGN INC ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL BERKELEY SOFTWARE DESIGN INC BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from BSDI $Id: mutex_witness.c,v 1.1.2.20 2000/04/27 03:10:27 cp Exp $
 *	and BSDI $Id: synch_machdep.c,v 2.3.2.39 2000/04/27 03:10:25 cp Exp $
 */

/*
 * Machine independent bits of mutex implementation.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: release/9.1.0/sys/kern/kern_mutex.c 236238 2012-05-29 14:50:21Z fabient $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/sbuf.h>
#include <sys/vmmeter.h>

#include <machine/atomic.h>
#include <machine/bus.h>
#include <machine/cpu.h>

#include <fs/devfs/devfs_int.h>

#include <vm/vm.h>
#include <vm/vm_extern.h>

#include "kos/BlockingSync.h"
#include "kos/Thread.h"

/*
 * Internal utility macros.
 */
#define mtx_unowned(m)	((m)->mtx_lock == MTX_UNOWNED)
#define	mtx_destroyed(m) ((m)->mtx_lock == MTX_DESTROYED)

static void	assert_mtx(struct lock_object *lock, int what);
static void	lock_mtx(struct lock_object *lock, int how);
static void	lock_spin(struct lock_object *lock, int how);
static int	unlock_mtx(struct lock_object *lock);
static int	unlock_spin(struct lock_object *lock);

/*
 * Lock classes for sleep and spin mutexes.
 */
struct lock_class lock_class_mtx_sleep = {
	.lc_name = "sleep mutex",
	.lc_flags = LC_SLEEPLOCK | LC_RECURSABLE,
	.lc_assert = assert_mtx,
	.lc_lock = lock_mtx,
	.lc_unlock = unlock_mtx,
};
struct lock_class lock_class_mtx_spin = {
	.lc_name = "spin mutex",
	.lc_flags = LC_SPINLOCK | LC_RECURSABLE,
	.lc_assert = assert_mtx,
	.lc_lock = lock_spin,
	.lc_unlock = unlock_spin,
};

/*
 * System-wide mutexes
 */
struct mtx blocked_lock;
struct mtx Giant;

void
assert_mtx(struct lock_object *lock, int what)
{

	mtx_assert((struct mtx *)lock, what);
}

void
lock_mtx(struct lock_object *lock, int how)
{

	mtx_lock((struct mtx *)lock);
}

void
lock_spin(struct lock_object *lock, int how)
{

	panic("spin locks can only use msleep_spin");
}

int
unlock_mtx(struct lock_object *lock)
{
	struct mtx *m;

	m = (struct mtx *)lock;
	mtx_assert(m, MA_OWNED | MA_NOTRECURSED);
	mtx_unlock(m);
	return (0);
}

int
unlock_spin(struct lock_object *lock)
{

	panic("spin locks can only use msleep_spin");
}

/*
 * Function versions of the inlined __mtx_* macros.  These are used by
 * modules and can also be called from assembly language if needed.
 */
void
_mtx_lock_flags(struct mtx *m, int opts, const char *file, int line)
{

	if (SCHEDULER_STOPPED())
		return;
  MPASS(KOS_CurThread() != NULL);
	KASSERT(m->mtx_lock != MTX_DESTROYED,
	    ("mtx_lock() of destroyed mutex @ %s:%d", file, line));
	KASSERT(LOCK_CLASS(&m->lock_object) == &lock_class_mtx_sleep,
	    ("mtx_lock() of spin mutex %s @ %s:%d", m->lock_object.lo_name,
	    file, line));

  KOS_MutexLock(m, opts, file, line);
}

void
_mtx_unlock_flags(struct mtx *m, int opts, const char *file, int line)
{

	if (SCHEDULER_STOPPED())
		return;
	MPASS(KOS_CurThread() != NULL);
	KASSERT(m->mtx_lock != MTX_DESTROYED,
	    ("mtx_unlock() of destroyed mutex @ %s:%d", file, line));
	KASSERT(LOCK_CLASS(&m->lock_object) == &lock_class_mtx_sleep,
	    ("mtx_unlock() of spin mutex %s @ %s:%d", m->lock_object.lo_name,
	    file, line));

  KOS_MutexUnLock(m, opts, file, line);
}

void
_mtx_lock_spin_flags(struct mtx *m, int opts, const char *file, int line)
{

	if (SCHEDULER_STOPPED())
		return;
	MPASS(KOS_CurThread() != NULL);
	KASSERT(m->mtx_lock != MTX_DESTROYED,
	    ("mtx_lock_spin() of destroyed mutex @ %s:%d", file, line));
	KASSERT(LOCK_CLASS(&m->lock_object) == &lock_class_mtx_spin,
	    ("mtx_lock_spin() of sleep mutex %s @ %s:%d",
	    m->lock_object.lo_name, file, line));

  KOS_MutexLock(m, opts, file, line);
}

void
_mtx_unlock_spin_flags(struct mtx *m, int opts, const char *file, int line)
{

	if (SCHEDULER_STOPPED())
		return;
	MPASS(KOS_CurThread() != NULL);
	KASSERT(m->mtx_lock != MTX_DESTROYED,
	    ("mtx_unlock_spin() of destroyed mutex @ %s:%d", file, line));
	KASSERT(LOCK_CLASS(&m->lock_object) == &lock_class_mtx_spin,
	    ("mtx_unlock_spin() of sleep mutex %s @ %s:%d",
	    m->lock_object.lo_name, file, line));
	mtx_assert(m, MA_OWNED);

  KOS_MutexUnLock(m, opts, file, line);
}

/*
 * The important part of mtx_trylock{,_flags}()
 * Tries to acquire lock `m.'  If this function is called on a mutex that
 * is already owned, it will recursively acquire the lock.
 */
int
_mtx_trylock(struct mtx *m, int opts, const char *file, int line)
{
	int rval;

	if (SCHEDULER_STOPPED())
		return (1);

	MPASS(KOS_CurThread() != NULL);
	KASSERT(m->mtx_lock != MTX_DESTROYED,
	    ("mtx_trylock() of destroyed mutex @ %s:%d", file, line));
	KASSERT(LOCK_CLASS(&m->lock_object) == &lock_class_mtx_sleep,
	    ("mtx_trylock() of spin mutex %s @ %s:%d", m->lock_object.lo_name,
	    file, line));

  rval = KOS_MutexTryLock(m, opts, file, line);
	return (rval);
}

void
_thread_lock_flags(void *td, int opts, const char *file, int line)
{
	if (SCHEDULER_STOPPED())
		return;

  KOS_ThreadLock(td, file, line);
}

void
_thread_unlock(void *td, const char *file, int line)
{
  KOS_ThreadUnLock(td, file, line);
}

/*
 * General init routine used by the MTX_SYSINIT() macro.
 */
void
mtx_sysinit(void *arg)
{
	struct mtx_args *margs = arg;

	mtx_init(margs->ma_mtx, margs->ma_desc, NULL, margs->ma_opts);
}

/*
 * Mutex initialization routine; initialize lock `m' of type contained in
 * `opts' with options contained in `opts' and name `name.'  The optional
 * lock type `type' is used as a general lock category name for use with
 * witness.
 */
void
mtx_init(struct mtx *m, const char *name, const char *type, int opts)
{
	struct lock_class *class;
	int flags;

	MPASS((opts & ~(MTX_SPIN | MTX_QUIET | MTX_RECURSE |
		MTX_NOWITNESS | MTX_DUPOK | MTX_NOPROFILE)) == 0);
	ASSERT_ATOMIC_LOAD_PTR(m->mtx_lock,
	    ("%s: mtx_lock not aligned for %s: %p", __func__, name,
	    &m->mtx_lock));

	/* Determine lock class and lock flags. */
	if (opts & MTX_SPIN)
		class = &lock_class_mtx_spin;
	else
		class = &lock_class_mtx_sleep;
	flags = 0;
	if (opts & MTX_QUIET)
		flags |= LO_QUIET;
	if (opts & MTX_RECURSE)
		flags |= LO_RECURSABLE;
	if ((opts & MTX_NOWITNESS) == 0)
		flags |= LO_WITNESS;
	if (opts & MTX_DUPOK)
		flags |= LO_DUPOK;
	if (opts & MTX_NOPROFILE)
		flags |= LO_NOPROFILE;

	/* Initialize mutex. */
	m->mtx_lock = MTX_UNOWNED;
	m->mtx_recurse = 0;

	lock_init(&m->lock_object, class, name, type, flags);
  KOS_MutexInit(m, flags, opts & MTX_SPIN);
}

/*
 * Remove lock `m' from all_mtx queue.  We don't allow MTX_QUIET to be
 * passed in as a flag here because if the corresponding mtx_init() was
 * called with MTX_QUIET set, then it will already be set in the mutex's
 * flags.
 */
void
mtx_destroy(struct mtx *m)
{
	m->mtx_lock = MTX_DESTROYED;
	lock_destroy(&m->lock_object);
}

/*
 * Intialize the mutex code and system mutexes.  This is called from the MD
 * startup code prior to mi_startup().  The per-CPU data space needs to be
 * setup before this is called.
 */
void
mutex_init(void)
{
	/*
	 * Initialize mutexes.
	 */
	mtx_init(&Giant, "Giant", NULL, MTX_DEF | MTX_RECURSE);
	mtx_init(&blocked_lock, "blocked lock", NULL, MTX_SPIN);
	blocked_lock.mtx_lock = 0xdeadc0de;	/* Always blocked. */
#ifdef SUKWON
	mtx_init(&devmtx, "cdev", NULL, MTX_DEF); // TODO uncomment later
#endif
	mtx_lock(&Giant);
}
