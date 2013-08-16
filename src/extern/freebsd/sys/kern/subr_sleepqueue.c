/*-
 * Copyright (c) 2004 John Baldwin <jhb@FreeBSD.org>
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
 * Implementation of sleep queues used to hold queue of threads blocked on
 * a wait channel.  Sleep queues different from turnstiles in that wait
 * channels are not owned by anyone, so there is no priority propagation.
 * Sleep queues can also provide a timeout and can also be interrupted by
 * signals.  That said, there are several similarities between the turnstile
 * and sleep queue implementations.  (Note: turnstiles were implemented
 * first.)  For example, both use a hash table of the same size where each
 * bucket is referred to as a "chain" that contains both a spin lock and
 * a linked list of queues.  An individual queue is located by using a hash
 * to pick a chain, locking the chain, and then walking the chain searching
 * for the queue.  This means that a wait channel object does not need to
 * embed it's queue head just as locks do not embed their turnstile queue
 * head.  Threads also carry around a sleep queue that they lend to the
 * wait channel when blocking.  Just as in turnstiles, the queue includes
 * a free list of the sleep queues of other threads blocked on the same
 * wait channel in the case of multiple waiters.
 *
 * Some additional functionality provided by sleep queues include the
 * ability to set a timeout.  The timeout is managed using a per-thread
 * callout that resumes a thread if it is asleep.  A thread may also
 * catch signals while it is asleep (aka an interruptible sleep).  The
 * signal code uses sleepq_abort() to interrupt a sleeping thread.  Finally,
 * sleep queues also provide some extra assertions.  One is not allowed to
 * mix the sleep/wakeup and cv APIs for a given wait channel.  Also, one
 * must consistently use the same lock to synchronize with a wait channel,
 * though this check is currently only a warning for sleep/wakeup due to
 * pre-existing abuse of that API.  The same lock must also be held when
 * awakening threads, though that is currently only enforced for condition
 * variables.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: release/9.1.0/sys/kern/subr_sleepqueue.c 236344 2012-05-30 23:22:52Z rstone $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/lock.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/sleepqueue.h>

#include "kos/Kassert.h"
#include "kos/Thread.h"
#include "kos/SleepQueue.h"
#include "kos/Callout.h"

struct timeout_data { // for callout arg
  void *td;
  void *wchan;
};

/*
 * Prototypes for non-exported routines.
 */
static int	sleepq_catch_signals(void *wchan, int pri);
static int	sleepq_check_signals(void);
static int	sleepq_check_timeout(void);
static int	sleepq_init(void *mem, int size, int flags);
static int	sleepq_resume_thread(struct sleepqueue *sq, struct thread *td,
		    int pri);
static void	sleepq_timeout(void *arg);

/*
 * Early initialization of sleep queues that is called from the sleepinit()
 * SYSINIT.
 */
void
init_sleepqueues(void)
{

}

/*
 * Lock the sleep queue chain associated with the specified wait channel.
 */
void
sleepq_lock(void *wchan)
{
  KOS_SleepQueueLock(); // acquire a global lock for sleepqueues
}

/*
 * Look up the sleep queue associated with a given wait channel in the hash
 * table locking the associated sleep queue chain.  If no queue is found in
 * the table, NULL is returned.
 */
void *
sleepq_lookup(void *wchan)
{
  KOS_SleepQueueLockAssert();
  return KOS_SleepQueueLookUpLocked(wchan);
}

/*
 * Unlock the sleep queue chain associated with a given wait channel.
 */
void
sleepq_release(void *wchan)
{
  KOS_SleepQueueUnLock();
}

/*
 * Places the current thread on the sleep queue for the specified wait
 * channel.  If INVARIANTS is enabled, then it associates the passed in
 * lock with the sleepq to make sure it is held when that sleep queue is
 * woken up.
 */
void
sleepq_add(void *wchan, struct lock_object *lock, const char *wmesg, int flags,
    int queue)
{
  void *sq;
  void *td;
  
  KOS_SleepQueueLockAssert();
	MPASS(wchan != NULL);
	MPASS((queue >= 0) && (queue < NR_SLEEPQS));

	/* Look up the sleep queue associated with the wait channel 'wchan'. */
	sq = sleepq_lookup(wchan);
  
  if (sq == NULL) {
    sq = KOS_SleepQueueCreateLocked(wchan);
  }

  td = KOS_CurThread();
  thread_lock(td);
  KOS_SleepQueueBlockThreadLocked(td, sq, wmesg); // add me as blocked on sleepqueue
	thread_unlock(td);
}

/*
 * Sets a timeout that will remove the current thread from the specified
 * sleep queue after timo ticks if the thread has not already been awakened.
 */
void
sleepq_set_timeout(void *wchan, int timo)
{
  void *td;
  KOS_SleepQueueLockAssert();
  BSD_KASSERTSTR((wchan != NULL), "null wchan");

  td = KOS_CurThread();
  if (KOS_GetPerThreadCallout(td) == NULL) {
    struct callout* co = malloc(sizeof(struct callout), M_TEMP, M_WAITOK);
    callout_init(co, CALLOUT_MPSAFE);
    KOS_SetPerThreadCallout(td, co);
  }
  struct timeout_data *tdata = malloc(sizeof(struct timeout_data), M_TEMP, M_WAITOK);
  tdata->td = td;
  tdata->wchan = wchan;
  callout_reset_curcpu(KOS_GetPerThreadCallout(td), timo, sleepq_timeout, tdata);
//  KOS_SleepQueueResetTimeout(wchan, timo);
}

/*
 * Return the number of actual sleepers for the specified queue.
 */
u_int
sleepq_sleepcnt(void *wchan, int queue)
{
  void *sq;

	KASSERT(wchan != NULL, ("%s: invalid NULL wait channel", __func__));
	MPASS((queue >= 0) && (queue < NR_SLEEPQS));
	sq = sleepq_lookup(wchan);
	if (sq == NULL)
		return (0);
  return KOS_SleepQueueBlockedThreadCount(sq);
}

/*
 * Check to see if we timed out.
 */
static int
sleepq_check_timeout(void)
{
  void *td = KOS_CurThread();
  // THREAD_LOCK_ASSERT(td, MA_OWNED)

  if (KOS_SleepQueueGetFlags(td) & TDF_TIMEOUT) {
    KOS_SleepQueueResetFlags(td, TDF_TIMEOUT);
    return EWOULDBLOCK;
  }

  if (callout_stop(KOS_GetPerThreadCallout(td)) == 0) {
    KOS_SleepQueueSetFlags(td, TDF_TIMEOUT);
    KOS_Suspend(td);
  }
  return 0;
}

/*
 * Block the current thread until it is awakened from its sleep queue.
 */
void
sleepq_wait(void *wchan, int pri)
{
	void *td;

	td = KOS_CurThread();
	thread_lock(td);
  KOS_SleepQueueWait(wchan, td);
	thread_unlock(td);
}

/*
 * Block the current thread until it is awakened from its sleep queue
 * or it is interrupted by a signal.
 */
int
sleepq_wait_sig(void *wchan, int pri)
{
  BSD_KASSERTSTR((false), "unimplemented sleepq_wait_sig");
  return 0;
}

/*
 * Block the current thread until it is awakened from its sleep queue
 * or it times out while waiting.
 */
int
sleepq_timedwait(void *wchan, int pri)
{
	void *td;
	int rval;

	td = KOS_CurThread();
	thread_lock(td);
  KOS_SleepQueueWait(wchan, td);
//  rval = KOS_SleepQueueCheckTimeout(wchan);
  rval = sleepq_check_timeout();
	thread_unlock(td);

	return (rval);
}

/*
 * Block the current thread until it is awakened from its sleep queue,
 * it is interrupted by a signal, or it times out waiting to be awakened.
 */
int
sleepq_timedwait_sig(void *wchan, int pri)
{
  BSD_KASSERTSTR((false), "unimplemented sleepq_timedwait_sig");
  return 0;
}

/*
 * Find the highest priority thread sleeping on a wait channel and resume it.
 */
int
sleepq_signal(void *wchan, int flags, int pri, int queue)
{
	void *sq;

	KASSERT(wchan != NULL, ("%s: invalid NULL wait channel", __func__));
	MPASS((queue >= 0) && (queue < NR_SLEEPQS));
	sq = sleepq_lookup(wchan);
	if (sq == NULL)
		return (0);

	/*
	 * Find the highest priority thread on the queue.  If there is a
	 * tie, use the thread that first appears in the queue as it has
	 * been sleeping the longest since threads are always added to
	 * the tail of sleep queues.
	 */
  KOS_SleepQueueWakeupOne(sq);
  return 0;
}

/*
 * Resume all threads sleeping on a specified wait channel.
 */
int
sleepq_broadcast(void *wchan, int flags, int pri, int queue)
{
  void *sq;

	KASSERT(wchan != NULL, ("%s: invalid NULL wait channel", __func__));
	MPASS((queue >= 0) && (queue < NR_SLEEPQS));
	sq = sleepq_lookup(wchan);
	if (sq == NULL)
		return (0);

	/* Resume all blocked threads on the sleep queue. */
  KOS_SleepQueueWakeup(sq);
  return 0;
}

/*
 * Time sleeping threads out.  When the timeout expires, the thread is
 * removed from the sleep queue and made runnable if it is still asleep.
 */
static void
sleepq_timeout(void *arg)
{
	void *td;
	void *wchan;
  struct timeout_data *tdata = (struct timeout_data *)arg;

  td = tdata->td;
  wchan = tdata->wchan;

	/*
	 * First, see if the thread is asleep and get the wait channel if
	 * it is.
	 */
	thread_lock(td);
  if (KOS_SleepQueueIsSleeping(td) && KOS_OnSleepQueue(td)) {
    KOS_SleepQueueSetFlags(td, TDF_TIMEOUT);
    KOS_SleepQueueWakeupThread(td, wchan);
    thread_unlock(td);
    return;
  }

	/*
	 * If the thread is on the SLEEPQ but isn't sleeping yet, it
	 * can either be on another CPU in between sleepq_add() and
	 * one of the sleepq_*wait*() routines or it can be in
	 * sleepq_catch_signals().
	 */
  if (KOS_OnSleepQueue(td)) {
    KOS_SleepQueueSetFlags(td, TDF_TIMEOUT);
    thread_unlock(td);
    return;
  }

	/*
	 * Now check for the edge cases.  First, if TDF_TIMEOUT is set,
	 * then the other thread has already yielded to us, so clear
	 * the flag and resume it.  If TDF_TIMEOUT is not set, then the
	 * we know that the other thread is not on a sleep queue, but it
	 * hasn't resumed execution yet.  In that case, set TDF_TIMOFAIL
	 * to let it know that the timeout has already run and doesn't
	 * need to be canceled.
	 */
  if (KOS_SleepQueueGetFlags(td) & TDF_TIMEOUT) {
    KOS_SleepQueueResetFlags(td, TDF_TIMEOUT);
    KOS_SleepQueueSetSleeping(td, 0);
    KOS_ScheduleLocked(td, 0);
  } else {
    KOS_SleepQueueSetFlags(td, TDF_TIMOFAIL);
  }
	thread_unlock(td);
}

/*
 * Resumes a specific thread from the sleep queue associated with a specific
 * wait channel if it is on that queue.
 */
void
sleepq_remove(void *td, void *wchan)
{
  KOS_SleepQueueWakeupThread(td, wchan);
}

/*
 * Abort a thread as if an interrupt had occurred.  Only abort
 * interruptible waits (unfortunately it isn't safe to abort others).
 */
int
sleepq_abort(void *td, int intrval)
{
  BSD_KASSERTSTR((false), "unimplemented sleepq_abort");
  return 0;
}
