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
 * Machine independent bits of reader/writer lock implementation.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: release/9.1.0/sys/kern/kern_rwlock.c 236238 2012-05-29 14:50:21Z fabient $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/rwlock.h>
#include <sys/systm.h>
#include "kos/ReadWriteLock.h"

#include <machine/cpu.h>

#if defined(SMP) && !defined(NO_ADAPTIVE_RWLOCKS)
#define	ADAPTIVE_RWLOCKS
#endif

#ifdef ADAPTIVE_RWLOCKS
#define	ROWNER_RETRIES	10
#define	ROWNER_LOOPS	10000
#endif

static void	assert_rw(struct lock_object *lock, int what);
static void	lock_rw(struct lock_object *lock, int how);
static int	unlock_rw(struct lock_object *lock);

struct lock_class lock_class_rw = {
	.lc_name = "rw",
	.lc_flags = LC_SLEEPLOCK | LC_RECURSABLE | LC_UPGRADABLE,
	.lc_assert = assert_rw,
	.lc_lock = lock_rw,
	.lc_unlock = unlock_rw,
};

/*
 * Return a pointer to the owning thread if the lock is write-locked or
 * NULL if the lock is unlocked or read-locked.
 */
#define	rw_wowner(rw)							\
	((rw)->rw_lock & RW_LOCK_READ ? NULL :				\
	    (struct thread *)RW_OWNER((rw)->rw_lock))

/*
 * Returns if a write owner is recursed.  Write ownership is not assured
 * here and should be previously checked.
 */
#define	rw_recursed(rw)		((rw)->rw_recurse != 0)

/*
 * Return true if curthread helds the lock.
 */
#define	rw_wlocked(rw)		(rw_wowner((rw)) == curthread)

/*
 * Return a pointer to the owning thread for this lock who should receive
 * any priority lent by threads that block on this lock.  Currently this
 * is identical to rw_wowner().
 */
#define	rw_owner(rw)		rw_wowner(rw)

#ifndef INVARIANTS
#define	_rw_assert(rw, what, file, line)
#endif

void
assert_rw(struct lock_object *lock, int what)
{

	rw_assert((struct rwlock *)lock, what);
}

void
lock_rw(struct lock_object *lock, int how)
{
	struct rwlock *rw;

	rw = (struct rwlock *)lock;
	if (how)
		rw_wlock(rw);
	else
		rw_rlock(rw);
}

int
unlock_rw(struct lock_object *lock)
{
	struct rwlock *rw;

	rw = (struct rwlock *)lock;
	rw_assert(rw, RA_LOCKED | LA_NOTRECURSED);
	if (rw->rw_lock & RW_LOCK_READ) {
		rw_runlock(rw);
		return (0);
	} else {
		rw_wunlock(rw);
		return (1);
	}
}

void
rw_init_flags(struct rwlock *rw, const char *name, int opts)
{
	int flags;

	MPASS((opts & ~(RW_DUPOK | RW_NOPROFILE | RW_NOWITNESS | RW_QUIET |
	    RW_RECURSE)) == 0);
	ASSERT_ATOMIC_LOAD_PTR(rw->rw_lock,
	    ("%s: rw_lock not aligned for %s: %p", __func__, name,
	    &rw->rw_lock));

	flags = LO_UPGRADABLE;
	if (opts & RW_DUPOK)
		flags |= LO_DUPOK;
	if (opts & RW_NOPROFILE)
		flags |= LO_NOPROFILE;
	if (!(opts & RW_NOWITNESS))
		flags |= LO_WITNESS;
	if (opts & RW_RECURSE)
		flags |= LO_RECURSABLE;
	if (opts & RW_QUIET)
		flags |= LO_QUIET;

	rw->rw_lock = RW_UNLOCKED;
	rw->rw_recurse = 0;
	lock_init(&rw->lock_object, &lock_class_rw, name, NULL, flags);
  KOS_RWLockInit(rw, flags);
}

void
rw_destroy(struct rwlock *rw)
{

	KASSERT(rw->rw_lock == RW_UNLOCKED, ("rw lock %p not unlocked", rw));
	KASSERT(rw->rw_recurse == 0, ("rw lock %p still recursed", rw));
	rw->rw_lock = RW_DESTROYED;
	lock_destroy(&rw->lock_object);
}

void
rw_sysinit(void *arg)
{
	struct rw_args *args = arg;

	rw_init(args->ra_rw, args->ra_desc);
}

void
rw_sysinit_flags(void *arg)
{
	struct rw_args_flags *args = arg;

	rw_init_flags(args->ra_rw, args->ra_desc, args->ra_flags);
}

int
rw_wowned(struct rwlock *rw)
{

  return KOS_RWLockOwned(rw);
}

void
_rw_wlock(struct rwlock *rw, const char *file, int line)
{

	if (SCHEDULER_STOPPED())
		return;
	KASSERT(rw->rw_lock != RW_DESTROYED,
	    ("rw_wlock() of destroyed rwlock @ %s:%d", file, line));
  KOS_RWLockWriteLock(rw, file, line);
}

int
_rw_try_wlock(struct rwlock *rw, const char *file, int line)
{
	if (SCHEDULER_STOPPED())
		return (1);

	KASSERT(rw->rw_lock != RW_DESTROYED,
	    ("rw_try_wlock() of destroyed rwlock @ %s:%d", file, line));

  return KOS_RWLockTryWriteLock(rw, file, line);
}

void
_rw_wunlock(struct rwlock *rw, const char *file, int line)
{

	if (SCHEDULER_STOPPED())
		return;
	KASSERT(rw->rw_lock != RW_DESTROYED,
	    ("rw_wunlock() of destroyed rwlock @ %s:%d", file, line));
  KOS_RWLockWriteUnLock(rw, file, line);
}

/*
 * Determines whether a new reader can acquire a lock.  Succeeds if the
 * reader already owns a read lock and the lock is locked for read to
 * prevent deadlock from reader recursion.  Also succeeds if the lock
 * is unlocked and has no writer waiters or spinners.  Failing otherwise
 * prioritizes writers before readers.
 */
#define	RW_CAN_READ(_rw)						\
    ((curthread->td_rw_rlocks && (_rw) & RW_LOCK_READ) || ((_rw) &	\
    (RW_LOCK_READ | RW_LOCK_WRITE_WAITERS | RW_LOCK_WRITE_SPINNER)) ==	\
    RW_LOCK_READ)

void
_rw_rlock(struct rwlock *rw, const char *file, int line)
{
	if (SCHEDULER_STOPPED())
		return;

	KASSERT(rw->rw_lock != RW_DESTROYED,
	    ("rw_rlock() of destroyed rwlock @ %s:%d", file, line));

  KOS_RWLockReadLock(rw, file, line);
}

int
_rw_try_rlock(struct rwlock *rw, const char *file, int line)
{
	if (SCHEDULER_STOPPED())
		return (1);

  return KOS_RWLockTryReadLock(rw, file, line);
}

void
_rw_runlock(struct rwlock *rw, const char *file, int line)
{
	if (SCHEDULER_STOPPED())
		return;

	KASSERT(rw->rw_lock != RW_DESTROYED,
	    ("rw_runlock() of destroyed rwlock @ %s:%d", file, line));

  KOS_RWLockReadUnLock(rw, file, line);
}

/*
 * This function is called when we are unable to obtain a write lock on the
 * first try.  This means that at least one other thread holds either a
 * read or write lock.
 */
void
_rw_wlock_hard(struct rwlock *rw, uintptr_t tid, const char *file, int line)
{
	if (SCHEDULER_STOPPED())
		return;

  KOS_RWLockWriteLock(rw, file, line);
}

/*
 * This function is called if the first try at releasing a write lock failed.
 * This means that one of the 2 waiter bits must be set indicating that at
 * least one thread is waiting on this lock.
 */
void
_rw_wunlock_hard(struct rwlock *rw, uintptr_t tid, const char *file, int line)
{
	if (SCHEDULER_STOPPED())
		return;

  KOS_RWLockWriteUnLock(rw, file, line);
}

/*
 * Attempt to do a non-blocking upgrade from a read lock to a write
 * lock.  This will only succeed if this thread holds a single read
 * lock.  Returns true if the upgrade succeeded and false otherwise.
 */
int
_rw_try_upgrade(struct rwlock *rw, const char *file, int line)
{
	if (SCHEDULER_STOPPED())
		return (1);

	KASSERT(rw->rw_lock != RW_DESTROYED,
	    ("rw_try_upgrade() of destroyed rwlock @ %s:%d", file, line));

  return KOS_RWLockTryUpgrade(rw, file, line);
}

/*
 * Downgrade a write lock into a single read lock.
 */
void
_rw_downgrade(struct rwlock *rw, const char *file, int line)
{
	if (SCHEDULER_STOPPED())
		return;

	KASSERT(rw->rw_lock != RW_DESTROYED,
	    ("rw_downgrade() of destroyed rwlock @ %s:%d", file, line));

  KOS_RWLockDowngrade(rw, file, line);
}
