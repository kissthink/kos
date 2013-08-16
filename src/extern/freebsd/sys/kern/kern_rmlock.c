/*-
 * Copyright (c) 2007 Stephan Uphoff <ups@FreeBSD.org>
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
__FBSDID("$FreeBSD: release/9.1.0/sys/kern/kern_rmlock.c 235404 2012-05-13 17:01:32Z avg $");

#include <sys/param.h>
#include <sys/systm.h>

#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/rmlock.h>
#include <machine/cpu.h>
#include "kos/ReadWriteLock.h"

#define RMPF_ONQUEUE	1
#define RMPF_SIGNAL	2

// TODO: Please Fix, this implementation is not complete

/*
 * To support usage of rmlock in CVs and msleep yet another list for the
 * priority tracker would be needed.  Using this lock for cv and msleep also
 * does not seem very useful
 */

static void	assert_rm(struct lock_object *lock, int what);
static void	lock_rm(struct lock_object *lock, int how);
static int	unlock_rm(struct lock_object *lock);

struct lock_class lock_class_rm = {
	.lc_name = "rm",
	.lc_flags = LC_SLEEPLOCK | LC_RECURSABLE,
	.lc_assert = assert_rm,
	.lc_lock = lock_rm,
	.lc_unlock = unlock_rm,
};

static void
assert_rm(struct lock_object *lock, int what)
{

	panic("assert_rm called");
}

static void
lock_rm(struct lock_object *lock, int how)
{

	panic("lock_rm called");
}

static int
unlock_rm(struct lock_object *lock)
{

	panic("unlock_rm called");
}

CTASSERT((RM_SLEEPABLE & LO_CLASSFLAGS) == RM_SLEEPABLE);

void
rm_init_flags(struct rmlock *rm, const char *name, int opts)
{
	int liflags;

	liflags = 0;
	if (!(opts & RM_NOWITNESS))
		liflags |= LO_WITNESS;
	if (opts & RM_RECURSE)
		liflags |= LO_RECURSABLE;
	if (opts & RM_SLEEPABLE) {
		liflags |= RM_SLEEPABLE;
		sx_init_flags(&rm->rm_lock_sx, "rmlock_sx", SX_RECURSE);
	} else
		mtx_init(&rm->rm_lock_mtx, name, "rmlock_mtx", MTX_NOWITNESS);
	lock_init(&rm->lock_object, &lock_class_rm, name, NULL, liflags);
  KOS_RWLockInit(rm, liflags);
}

void
rm_init(struct rmlock *rm, const char *name)
{

	rm_init_flags(rm, name, 0);
}

void
rm_destroy(struct rmlock *rm)
{

	if (rm->lock_object.lo_flags & RM_SLEEPABLE)
		sx_destroy(&rm->rm_lock_sx);
	else
		mtx_destroy(&rm->rm_lock_mtx);
	lock_destroy(&rm->lock_object);
}

int
rm_wowned(struct rmlock *rm)
{
  return KOS_RWLockOwned(rm);
}

void
rm_sysinit(void *arg)
{
	struct rm_args *args = arg;

	rm_init(args->ra_rm, args->ra_desc);
}

void
rm_sysinit_flags(void *arg)
{
	struct rm_args_flags *args = arg;

	rm_init_flags(args->ra_rm, args->ra_desc, args->ra_opts);
}

int
_rm_rlock(struct rmlock *rm, struct rm_priotracker *tracker, int trylock)
{
	if (SCHEDULER_STOPPED())
		return (1);

  KOS_RWLockReadLock(rm, __FILE__, __LINE__);
  return 1;
}

void
_rm_runlock(struct rmlock *rm, struct rm_priotracker *tracker)
{
	if (SCHEDULER_STOPPED())
		return;

  KOS_RWLockReadUnLock(rm, __FILE__, __LINE__);
}

void
_rm_wlock(struct rmlock *rm)
{
	if (SCHEDULER_STOPPED())
		return;

  KOS_RWLockWriteLock(rm, __FILE__, __LINE__);
}

void
_rm_wunlock(struct rmlock *rm)
{
  KOS_RWLockWriteUnLock(rm, __FILE__, __LINE__);
}

/*
 * Just strip out file and line arguments if no lock debugging is enabled in
 * the kernel - we are called from a kernel module.
 */
void
_rm_wlock_debug(struct rmlock *rm, const char *file, int line)
{

	_rm_wlock(rm);
}

void
_rm_wunlock_debug(struct rmlock *rm, const char *file, int line)
{

	_rm_wunlock(rm);
}

int
_rm_rlock_debug(struct rmlock *rm, struct rm_priotracker *tracker,
    int trylock, const char *file, int line)
{

	return _rm_rlock(rm, tracker, trylock);
}

void
_rm_runlock_debug(struct rmlock *rm, struct rm_priotracker *tracker,
    const char *file, int line)
{

	_rm_runlock(rm, tracker);
}
