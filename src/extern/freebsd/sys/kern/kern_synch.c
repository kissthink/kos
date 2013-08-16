/*-
 * Copyright (c) 1982, 1986, 1990, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)kern_synch.c	8.9 (Berkeley) 5/19/95
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: release/9.1.0/sys/kern/kern_synch.c 237719 2012-06-28 18:38:24Z jhb $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/condvar.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/signalvar.h>
#include <sys/sleepqueue.h>
#include <sys/smp.h>
#include <sys/sx.h>
#include <sys/sysproto.h>
#include <sys/vmmeter.h>
#include "kos/Kassert.h"
#include "kos/SleepQueue.h"
#include "kos/Thread.h"
#include "kos/Timer.h"

#include <machine/cpu.h>

static void synch_setup(void *dummy);
SYSINIT(synch_setup, SI_SUB_KICK_SCHEDULER, SI_ORDER_FIRST, synch_setup,
    NULL);

int	hogticks;
static int pause_wchan;

void
sleepinit(void)
{

	hogticks = (hz / 10) * 2;	/* Default only. */
	init_sleepqueues();
}

/*
 * General sleep call.  Suspends the current thread until a wakeup is
 * performed on the specified identifier.  The thread will then be made
 * runnable with the specified priority.  Sleeps at most timo/hz seconds
 * (0 means no timeout).  If pri includes PCATCH flag, signals are checked
 * before and after sleeping, else signals are not checked.  Returns 0 if
 * awakened, EWOULDBLOCK if the timeout expires.  If PCATCH is set and a
 * signal needs to be delivered, ERESTART is returned if the current system
 * call should be restarted if possible, and EINTR is returned if the system
 * call should be interrupted by the signal (return EINTR).
 *
 * The lock argument is unlocked before the caller is suspended, and
 * re-locked before _sleep() returns.  If priority includes the PDROP
 * flag the lock is not re-locked before returning.
 */
int
_sleep(void *ident, struct lock_object *lock, int priority,
    const char *wmesg, int timo)
{
	void *td;
	struct lock_class *class;
	int catch, flags, lock_state, pri, rval;

	td = KOS_CurThread();
	KASSERT(timo != 0 || mtx_owned(&Giant) || lock != NULL,
	    ("sleeping without a lock"));
	KASSERT(p != NULL, ("msleep1"));
	KASSERT(ident != NULL, ("msleep"));
	if (priority & PDROP)
		KASSERT(lock != NULL && lock != &Giant.lock_object,
		    ("PDROP requires a non-Giant lock"));
	if (lock != NULL)
		class = LOCK_CLASS(lock);
	else
		class = NULL;

	if (cold || SCHEDULER_STOPPED()) {
		/*
		 * During autoconfiguration, just return;
		 * don't run any other threads or panic below,
		 * in case this is the idle thread and already asleep.
		 * XXX: this used to do "s = splhigh(); splx(safepri);
		 * splx(s);" to give interrupts a chance, but there is
		 * no way to give interrupts a chance now.
		 */
		if (lock != NULL && priority & PDROP)
			class->lc_unlock(lock);
		return (0);
	}
	catch = priority & PCATCH;
	pri = priority & PRIMASK;

	/*
	 * If we are already on a sleep queue, then remove us from that
	 * sleep queue first.  We have to do this to handle recursive
	 * sleeps.
	 */
	if (KOS_OnSleepQueue(td))
		sleepq_remove(td, KOS_SleepQueueGetChannel(td));

	if (ident == &pause_wchan)
		flags = SLEEPQ_PAUSE;
	else
		flags = SLEEPQ_SLEEP;
	if (catch)
		flags |= SLEEPQ_INTERRUPTIBLE;
	if (priority & PBDRY)
		flags |= SLEEPQ_STOP_ON_BDRY;

	sleepq_lock(ident);

	if (lock == &Giant.lock_object)
		mtx_assert(&Giant, MA_OWNED);
	DROP_GIANT();
	if (lock != NULL && lock != &Giant.lock_object &&
	    !(class->lc_flags & LC_SLEEPABLE)) {
		lock_state = class->lc_unlock(lock);
	} else
		/* GCC needs to follow the Yellow Brick Road */
		lock_state = -1;

	/*
	 * We put ourselves on the sleep queue and start our timeout
	 * before calling thread_suspend_check, as we could stop there,
	 * and a wakeup or a SIGCONT (or both) could occur while we were
	 * stopped without resuming us.  Thus, we must be ready for sleep
	 * when cursig() is called.  If the wakeup happens while we're
	 * stopped, then td will no longer be on a sleep queue upon
	 * return from cursig().
	 */
	sleepq_add(ident, lock, wmesg, flags, 0);
	if (timo)
		sleepq_set_timeout(ident, timo);
	if (lock != NULL && class->lc_flags & LC_SLEEPABLE) {
		sleepq_release(ident);
		lock_state = class->lc_unlock(lock);
		sleepq_lock(ident);
	}
	if (timo && catch) {
      BSD_KASSERTSTR((false), "unimplemented sleepq_timedwait_sig");
//		rval = sleepq_timedwait_sig(ident, pri);
      rval = 0;
	} else if (timo) {
		rval = sleepq_timedwait(ident, pri);
	} else if (catch) {
      BSD_KASSERTSTR((false), "unimplemented sleepq_wait_sig");
//		rval = sleepq_wait_sig(ident, pri);
      rval = 0;
	} else {
		sleepq_wait(ident, pri);
		rval = 0;
	}
	PICKUP_GIANT();
	if (lock != NULL && lock != &Giant.lock_object && !(priority & PDROP)) {
		class->lc_lock(lock, lock_state);
	}
	return (rval);
}

int
msleep_spin(void *ident, struct mtx *mtx, const char *wmesg, int timo)
{
	int rval;

	KASSERT(mtx != NULL, ("sleeping without a mutex"));
	KASSERT(ident != NULL, ("msleep"));

	if (cold || SCHEDULER_STOPPED()) {
		/*
		 * During autoconfiguration, just return;
		 * don't run any other threads or panic below,
		 * in case this is the idle thread and already asleep.
		 * XXX: this used to do "s = splhigh(); splx(safepri);
		 * splx(s);" to give interrupts a chance, but there is
		 * no way to give interrupts a chance now.
		 */
		return (0);
	}

	sleepq_lock(ident);

	DROP_GIANT();
	mtx_assert(mtx, MA_OWNED | MA_NOTRECURSED);
	mtx_unlock_spin(mtx);

	/*
	 * We put ourselves on the sleep queue and start our timeout.
	 */
	sleepq_add(ident, &mtx->lock_object, wmesg, SLEEPQ_SLEEP, 0);
	if (timo)
		sleepq_set_timeout(ident, timo);

	if (timo)
		rval = sleepq_timedwait(ident, 0);
	else {
		sleepq_wait(ident, 0);
		rval = 0;
	}
	PICKUP_GIANT();
	mtx_lock_spin(mtx);
	return (rval);
}

/*
 * pause() delays the calling thread by the given number of system ticks.
 * During cold bootup, pause() uses the DELAY() function instead of
 * the tsleep() function to do the waiting. The "timo" argument must be
 * greater than or equal to zero. A "timo" value of zero is equivalent
 * to a "timo" value of one.
 */
int
pause(const char *wmesg, int timo)
{
	KASSERT(timo >= 0, ("pause: timo must be >= 0"));

	/* silently convert invalid timeouts */
	if (timo < 1)
		timo = 1;

#if 0
	if (cold) {
		/*
		 * We delay one HZ at a time to avoid overflowing the
		 * system specific DELAY() function(s):
		 */
		while (timo >= hz) {
			DELAY(1000000);
			timo -= hz;
		}
		if (timo > 0)
			DELAY(timo * tick);
		return (0);
	}
#endif
	return (tsleep(&pause_wchan, 0, wmesg, timo));
}

/*
 * Make all threads sleeping on the specified identifier runnable.
 */
void
wakeup(void *ident)
{
	sleepq_lock(ident);
	sleepq_broadcast(ident, SLEEPQ_SLEEP, 0, 0);
	sleepq_release(ident);
}

/*
 * Make a thread sleeping on the specified identifier runnable.
 * May wake more than one thread if a target thread is currently
 * swapped out.
 */
void
wakeup_one(void *ident)
{
	sleepq_lock(ident);
	sleepq_signal(ident, SLEEPQ_SLEEP, 0, 0);
	sleepq_release(ident);
}

/* ARGSUSED */
static void
synch_setup(void *dummy)
{

}

int
should_yield(void)
{

	return (KOS_Ticks() - KOS_GetVolTick(KOS_CurThread()) >= hogticks);
}

void
maybe_yield(void)
{

	if (should_yield())
		kern_yield(PRI_USER);
}

void
kern_yield(int prio)
{
	void *td;

	td = KOS_CurThread();
	DROP_GIANT();
	thread_lock(td);
  KOS_UpdateVolTick(td);
  KOS_Yield();
	thread_unlock(td);
	PICKUP_GIANT();
}
