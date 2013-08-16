/*-
 * Copyright (c) 2007 Attilio Rao <attilio@freebsd.org>
 * Copyright (c) 2001 Jason Evans <jasone@freebsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice(s), this list of conditions and the following disclaimer as
 *    the first lines of this file unmodified other than the possible
 *    addition of one or more copyright notices.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice(s), this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

/*
 * Shared/exclusive locks.  This implementation attempts to ensure
 * deterministic lock granting behavior, so that slocks and xlocks are
 * interleaved.
 *
 * Priority propagation will not generally raise the priority of lock holders,
 * so should not be relied upon in combination with sx locks.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: release/9.1.0/sys/kern/kern_sx.c 236238 2012-05-29 14:50:21Z fabient $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/sx.h>
#include "kos/ReadWriteLock.h"

#if defined(SMP) && !defined(NO_ADAPTIVE_SX)
#include <machine/cpu.h>
#endif

#if defined(SMP) && !defined(NO_ADAPTIVE_SX)
#define	ADAPTIVE_SX
#endif

CTASSERT((SX_NOADAPTIVE & LO_CLASSFLAGS) == SX_NOADAPTIVE);

/*
 * Returns true if an exclusive lock is recursed.  It assumes
 * curthread currently has an exclusive lock.
 */
#define	sx_recurse		lock_object.lo_data
#define	sx_recursed(sx)		((sx)->sx_recurse != 0)

static void	assert_sx(struct lock_object *lock, int what);
static void	lock_sx(struct lock_object *lock, int how);
static int	unlock_sx(struct lock_object *lock);

struct lock_class lock_class_sx = {
	.lc_name = "sx",
	.lc_flags = LC_SLEEPLOCK | LC_SLEEPABLE | LC_RECURSABLE | LC_UPGRADABLE,
	.lc_assert = assert_sx,
	.lc_lock = lock_sx,
	.lc_unlock = unlock_sx,
};

#ifndef INVARIANTS
#define	_sx_assert(sx, what, file, line)
#endif

void
assert_sx(struct lock_object *lock, int what)
{

}

void
lock_sx(struct lock_object *lock, int how)
{
	struct sx *sx;

	sx = (struct sx *)lock;
	if (how)
		sx_xlock(sx);
	else
		sx_slock(sx);
}

int
unlock_sx(struct lock_object *lock)
{
	struct sx *sx;

	sx = (struct sx *)lock;
	if (sx_xlocked(sx)) {
		sx_xunlock(sx);
		return (1);
	} else {
		sx_sunlock(sx);
		return (0);
	}
}

void
sx_sysinit(void *arg)
{
	struct sx_args *sargs = arg;

	sx_init_flags(sargs->sa_sx, sargs->sa_desc, sargs->sa_flags);
}

void
sx_init_flags(struct sx *sx, const char *description, int opts)
{
	int flags;

	MPASS((opts & ~(SX_QUIET | SX_RECURSE | SX_NOWITNESS | SX_DUPOK |
	    SX_NOPROFILE | SX_NOADAPTIVE)) == 0);
	ASSERT_ATOMIC_LOAD_PTR(sx->sx_lock,
	    ("%s: sx_lock not aligned for %s: %p", __func__, description,
	    &sx->sx_lock));

	flags = LO_SLEEPABLE | LO_UPGRADABLE;
	if (opts & SX_DUPOK)
		flags |= LO_DUPOK;
	if (opts & SX_NOPROFILE)
		flags |= LO_NOPROFILE;
	if (!(opts & SX_NOWITNESS))
		flags |= LO_WITNESS;
	if (opts & SX_RECURSE)
		flags |= LO_RECURSABLE;
	if (opts & SX_QUIET)
		flags |= LO_QUIET;

	flags |= opts & SX_NOADAPTIVE;
	sx->sx_lock = SX_LOCK_UNLOCKED;
	sx->sx_recurse = 0;
	lock_init(&sx->lock_object, &lock_class_sx, description, NULL, flags);
  KOS_RWLockInit(sx, flags);
}

void
sx_destroy(struct sx *sx)
{

	KASSERT(sx->sx_lock == SX_LOCK_UNLOCKED, ("sx lock still held"));
	KASSERT(sx->sx_recurse == 0, ("sx lock still recursed"));
	sx->sx_lock = SX_LOCK_DESTROYED;
	lock_destroy(&sx->lock_object);
}

int
_sx_slock(struct sx *sx, int opts, const char *file, int line)
{
	int error = 0;

	if (SCHEDULER_STOPPED())
		return (0);
	KASSERT(sx->sx_lock != SX_LOCK_DESTROYED,
	    ("sx_slock() of destroyed sx @ %s:%d", file, line));
	error = _sx_slock_hard(sx, opts, file, line);

	return (error);
}

int
_sx_try_slock(struct sx *sx, const char *file, int line)
{
	if (SCHEDULER_STOPPED())
		return (1);

  return KOS_RWLockTryReadLock(sx, file, line);
}

int
_sx_xlock(struct sx *sx, int opts, const char *file, int line)
{
	int error = 0;

	if (SCHEDULER_STOPPED())
		return (0);
	KASSERT(sx->sx_lock != SX_LOCK_DESTROYED,
	    ("sx_xlock() of destroyed sx @ %s:%d", file, line));
	error = _sx_xlock_hard(sx, 0 /* unused */, opts, file, line);

	return (error);
}

int
_sx_try_xlock(struct sx *sx, const char *file, int line)
{
	if (SCHEDULER_STOPPED())
		return (1);

	KASSERT(sx->sx_lock != SX_LOCK_DESTROYED,
	    ("sx_try_xlock() of destroyed sx @ %s:%d", file, line));

  return KOS_RWLockTryWriteLock(sx, file, line);
}

void
_sx_sunlock(struct sx *sx, const char *file, int line)
{
	if (SCHEDULER_STOPPED())
		return;
	KASSERT(sx->sx_lock != SX_LOCK_DESTROYED,
	    ("sx_sunlock() of destroyed sx @ %s:%d", file, line));
	_sx_sunlock_hard(sx, file, line);
}

void
_sx_xunlock(struct sx *sx, const char *file, int line)
{
	if (SCHEDULER_STOPPED())
		return;
	KASSERT(sx->sx_lock != SX_LOCK_DESTROYED,
	    ("sx_xunlock() of destroyed sx @ %s:%d", file, line));
	_sx_xunlock_hard(sx, 0 /* unused */, file, line);
}

/*
 * Try to do a non-blocking upgrade from a shared lock to an exclusive lock.
 * This will only succeed if this thread holds a single shared lock.
 * Return 1 if if the upgrade succeed, 0 otherwise.
 */
int
_sx_try_upgrade(struct sx *sx, const char *file, int line)
{
	if (SCHEDULER_STOPPED())
		return (1);

	KASSERT(sx->sx_lock != SX_LOCK_DESTROYED,
	    ("sx_try_upgrade() of destroyed sx @ %s:%d", file, line));

  return KOS_RWLockTryUpgrade(sx, file, line);
}

/*
 * Downgrade an unrecursed exclusive lock into a single shared lock.
 */
void
_sx_downgrade(struct sx *sx, const char *file, int line)
{
	if (SCHEDULER_STOPPED())
		return;

	KASSERT(sx->sx_lock != SX_LOCK_DESTROYED,
	    ("sx_downgrade() of destroyed sx @ %s:%d", file, line));

  KOS_RWLockDowngrade(sx, file, line);
}

/*
 * This function represents the so-called 'hard case' for sx_xlock
 * operation.  All 'easy case' failures are redirected to this.  Note
 * that ideally this would be a static function, but it needs to be
 * accessible from at least sx.h.
 */
int
_sx_xlock_hard(struct sx *sx, uintptr_t tid, int opts, const char *file,
    int line)
{
	if (SCHEDULER_STOPPED())
		return (0);

  KOS_RWLockWriteLock(sx, file, line);
  return 0; // XXX may return error once interruptible wait is implemented
}

/*
 * This function represents the so-called 'hard case' for sx_xunlock
 * operation.  All 'easy case' failures are redirected to this.  Note
 * that ideally this would be a static function, but it needs to be
 * accessible from at least sx.h.
 */
void
_sx_xunlock_hard(struct sx *sx, uintptr_t tid, const char *file, int line)
{
	if (SCHEDULER_STOPPED())
		return;

  KOS_RWLockWriteUnLock(sx, file, line);
}

/*
 * This function represents the so-called 'hard case' for sx_slock
 * operation.  All 'easy case' failures are redirected to this.  Note
 * that ideally this would be a static function, but it needs to be
 * accessible from at least sx.h.
 */
int
_sx_slock_hard(struct sx *sx, int opts, const char *file, int line)
{
	if (SCHEDULER_STOPPED())
		return (0);

  KOS_RWLockReadLock(sx, file, line);
  return 0; // XXX may return error once interruptible wait is implemented
}

/*
 * This function represents the so-called 'hard case' for sx_sunlock
 * operation.  All 'easy case' failures are redirected to this.  Note
 * that ideally this would be a static function, but it needs to be
 * accessible from at least sx.h.
 */
void
_sx_sunlock_hard(struct sx *sx, const char *file, int line)
{
	if (SCHEDULER_STOPPED())
		return;

  KOS_RWLockReadUnLock(sx, file, line);
}
