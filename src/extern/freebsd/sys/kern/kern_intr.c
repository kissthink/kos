/*-
 * Copyright (c) 1997, Stefan Esser <se@freebsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: release/9.1.0/sys/kern/kern_intr.c 239437 2012-08-20 15:19:34Z kan $");

#ifdef SUKWON
#include "opt_ddb.h"
#endif

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/cpuset.h>
#include <sys/rtprio.h>
#include <sys/systm.h>
#include <sys/interrupt.h>
#include <sys/kernel.h>
#ifdef SUKWON
#include <sys/kthread.h>
#endif
#include <sys/ktr.h>
#include <sys/limits.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/mutex.h>
#include <sys/priv.h>
#include <sys/proc.h>
#include <sys/random.h>
#ifdef SUKWON
#include <sys/resourcevar.h>
#endif
#include <sys/sched.h>
#include <sys/smp.h>
#include <sys/sysctl.h>
#ifdef SUKWON
#include <sys/syslog.h>
#endif
#include <sys/unistd.h>
#include <sys/vmmeter.h>
#include <machine/atomic.h>
#include <machine/cpu.h>
#include <machine/md_var.h>
#include <machine/stdarg.h>

#ifndef SUKWON
#include "kos/Thread.h"
//#undef thread_lock
//#undef thread_unlock
#undef TD_AWAITING_INTR
#undef TD_SET_IWAIT
#undef TD_CLR_IWAIT
#undef THREAD_NO_SLEEPING   // TODO implement later
#undef THREAD_SLEEPING_OK

//#define thread_lock(tdp)      KOS_ThreadLock((tdp))
//#define thread_unlock(tdp)    KOS_ThreadUnLock((tdp))
#define TD_AWAITING_INTR(td)  KOS_AWAITING_INTR((td), TDI_IWAIT)
#define TD_SET_IWAIT(td)      KOS_TD_SET_IWAIT((td), TDI_IWAIT)
#define TD_CLR_IWAIT(td)      KOS_TD_CLR_IWAIT((td), TDI_IWAIT)
#endif

/*
 * Describe an interrupt thread.  There is one of these per interrupt event.
 */
struct intr_thread {
	struct intr_event *it_event;
#ifdef SUKWON
	struct thread *it_thread;	/* Kernel thread. */
#else
  void *it_thread;/* KOS thread */
#endif
	int	it_flags;		/* (j) IT_* flags. */
	int	it_need;		/* Needs service. */
  int td_state;   /* not used */
  int td_inhibitors;  /* for TD_AWAITING_INTR() */
};

/* Interrupt thread flags kept in it_flags */
#define	IT_DEAD		0x000001	/* Thread is waiting to exit. */
#define	IT_WAIT		0x000002	/* Thread is waiting for completion. */

struct	intr_event *clk_intr_event;
struct	intr_event *tty_intr_event;
void	*vm_ih;
struct proc *intrproc;

//#ifdef SUKWON
static MALLOC_DEFINE(M_ITHREAD, "ithread", "Interrupt Threads");
//#else
//#define M_ITHREAD 0
//#endif

#ifdef SUKWON
static int intr_storm_threshold = 1000;
#endif
static TAILQ_HEAD(, intr_event) event_list =
    TAILQ_HEAD_INITIALIZER(event_list);
static struct mtx event_lock;
MTX_SYSINIT(intr_event_list, &event_lock, "intr event list", MTX_DEF);

static void	intr_event_update(struct intr_event *ie);
static int	intr_event_schedule_thread(struct intr_event *ie);
static struct intr_thread *ithread_create(const char *name);
static void	ithread_destroy(struct intr_thread *ithread);
static void	ithread_execute_handlers(struct intr_event *ie);
static void	ithread_loop(void *);
static void	ithread_update(struct intr_thread *ithd);
static void	start_softintr(void *);

/* Map an interrupt type to an ithread priority. */
u_char
intr_priority(enum intr_type flags)
{
	u_char pri;

	flags &= (INTR_TYPE_TTY | INTR_TYPE_BIO | INTR_TYPE_NET |
	    INTR_TYPE_CAM | INTR_TYPE_MISC | INTR_TYPE_CLK | INTR_TYPE_AV);
	switch (flags) {
	case INTR_TYPE_TTY:
		pri = PI_TTY;
		break;
	case INTR_TYPE_BIO:
		pri = PI_DISK;
		break;
	case INTR_TYPE_NET:
		pri = PI_NET;
		break;
	case INTR_TYPE_CAM:
		pri = PI_DISK;
		break;
	case INTR_TYPE_AV:
		pri = PI_AV;
		break;
	case INTR_TYPE_CLK:
		pri = PI_REALTIME;
		break;
	case INTR_TYPE_MISC:
		pri = PI_DULL;          /* don't care */
		break;
	default:
		/* We didn't specify an interrupt level. */
		panic("intr_priority: no interrupt type in flags");
	}

	return pri;
}

/*
 * Update an ithread based on the associated intr_event.
 */
static void
ithread_update(struct intr_thread *ithd)
{
	struct intr_event *ie;
	void *td;
#ifdef SUKWON
	u_char pri;
#endif

	ie = ithd->it_event;
	td = ithd->it_thread;

#ifdef SUKWON
	/* Determine the overall priority of this event. */
	if (TAILQ_EMPTY(&ie->ie_handlers))
		pri = PRI_MAX_ITHD;
	else
		pri = TAILQ_FIRST(&ie->ie_handlers)->ih_pri;
#endif

	/* Update name and priority. */
#ifdef SUKWON
	strlcpy(td->td_name, ie->ie_fullname, sizeof(td->td_name));
#else
  KOS_SetThreadName(td, ie->ie_fullname);
#endif
#ifdef SUKWON
	thread_lock(td);
	sched_prio(td, pri);
	thread_unlock(td);
#endif
}

/*
 * Regenerate the full name of an interrupt event and update its priority.
 */
static void
intr_event_update(struct intr_event *ie)
{
	struct intr_handler *ih;
	char *last;
	int missed, space;

	/* Start off with no entropy and just the name of the event. */
	mtx_assert(&ie->ie_lock, MA_OWNED);
	strlcpy(ie->ie_fullname, ie->ie_name, sizeof(ie->ie_fullname));
	ie->ie_flags &= ~IE_ENTROPY;
	missed = 0;
	space = 1;

	/* Run through all the handlers updating values. */
	TAILQ_FOREACH(ih, &ie->ie_handlers, ih_next) {
		if (strlen(ie->ie_fullname) + strlen(ih->ih_name) + 1 <
		    sizeof(ie->ie_fullname)) {
			strcat(ie->ie_fullname, " ");
			strcat(ie->ie_fullname, ih->ih_name);
			space = 0;
		} else
			missed++;
		if (ih->ih_flags & IH_ENTROPY)
			ie->ie_flags |= IE_ENTROPY;
	}

	/*
	 * If the handler names were too long, add +'s to indicate missing
	 * names. If we run out of room and still have +'s to add, change
	 * the last character from a + to a *.
	 */
	last = &ie->ie_fullname[sizeof(ie->ie_fullname) - 2];
	while (missed-- > 0) {
		if (strlen(ie->ie_fullname) + 1 == sizeof(ie->ie_fullname)) {
			if (*last == '+') {
				*last = '*';
				break;
			} else
				*last = '+';
		} else if (space) {
			strcat(ie->ie_fullname, " +");
			space = 0;
		} else
			strcat(ie->ie_fullname, "+");
	}

	/*
	 * If this event has an ithread, update it's priority and
	 * name.
	 */
	if (ie->ie_thread != NULL)
		ithread_update(ie->ie_thread);
}

int
intr_event_create(struct intr_event **event, void *source, int flags, int irq,
    void (*pre_ithread)(void *), void (*post_ithread)(void *),
    void (*post_filter)(void *), int (*assign_cpu)(void *, u_char),
    const char *fmt, ...)
{
	struct intr_event *ie;
	va_list ap;

	/* The only valid flag during creation is IE_SOFT. */
	if ((flags & ~IE_SOFT) != 0)
		return (EINVAL);
	ie = malloc(sizeof(struct intr_event), M_ITHREAD, M_WAITOK | M_ZERO);
	ie->ie_source = source;
	ie->ie_pre_ithread = pre_ithread;
	ie->ie_post_ithread = post_ithread;
	ie->ie_post_filter = post_filter;
	ie->ie_assign_cpu = assign_cpu;
	ie->ie_flags = flags;
	ie->ie_irq = irq;
	ie->ie_cpu = NOCPU;
	TAILQ_INIT(&ie->ie_handlers);
	mtx_init(&ie->ie_lock, "intr event", NULL, MTX_DEF);

	va_start(ap, fmt);
	vsnprintf(ie->ie_name, sizeof(ie->ie_name), fmt, ap);
	va_end(ap);
	strlcpy(ie->ie_fullname, ie->ie_name, sizeof(ie->ie_fullname));
	mtx_lock(&event_lock);
	TAILQ_INSERT_TAIL(&event_list, ie, ie_list);
	mtx_unlock(&event_lock);
	if (event != NULL)
		*event = ie;
	return (0);
}

/*
 * Bind an interrupt event to the specified CPU.  Note that not all
 * platforms support binding an interrupt to a CPU.  For those
 * platforms this request will fail.  For supported platforms, any
 * associated ithreads as well as the primary interrupt context will
 * be bound to the specificed CPU.  Using a cpu id of NOCPU unbinds
 * the interrupt event.
 */
int
intr_event_bind(struct intr_event *ie, u_char cpu)
{
#ifndef SUKWON
  return EOPNOTSUPP;
#else
	cpuset_t mask;
	lwpid_t id;
	int error;

	/* Need a CPU to bind to. */
	if (cpu != NOCPU && CPU_ABSENT(cpu))
		return (EINVAL);

	if (ie->ie_assign_cpu == NULL)
		return (EOPNOTSUPP);

	error = priv_check(curthread, PRIV_SCHED_CPUSET_INTR);
	if (error)
		return (error);

	/*
	 * If we have any ithreads try to set their mask first to verify
	 * permissions, etc.
	 */
	mtx_lock(&ie->ie_lock);
	if (ie->ie_thread != NULL) {
		CPU_ZERO(&mask);
		if (cpu == NOCPU)
			CPU_COPY(cpuset_root, &mask);
		else
			CPU_SET(cpu, &mask);
		id = ie->ie_thread->it_thread->td_tid;
		mtx_unlock(&ie->ie_lock);
		error = cpuset_setthread(id, &mask);
		if (error)
			return (error);
	} else
		mtx_unlock(&ie->ie_lock);
	error = ie->ie_assign_cpu(ie->ie_source, cpu);
	if (error) {
		mtx_lock(&ie->ie_lock);
		if (ie->ie_thread != NULL) {
			CPU_ZERO(&mask);
			if (ie->ie_cpu == NOCPU)
				CPU_COPY(cpuset_root, &mask);
			else
				CPU_SET(cpu, &mask);
			id = ie->ie_thread->it_thread->td_tid;
			mtx_unlock(&ie->ie_lock);
			(void)cpuset_setthread(id, &mask);
		} else
			mtx_unlock(&ie->ie_lock);
		return (error);
	}

	mtx_lock(&ie->ie_lock);
	ie->ie_cpu = cpu;
	mtx_unlock(&ie->ie_lock);

	return (error);
#endif
}

static struct intr_event *
intr_lookup(int irq)
{
	struct intr_event *ie;

	mtx_lock(&event_lock);
	TAILQ_FOREACH(ie, &event_list, ie_list)
		if (ie->ie_irq == irq &&
		    (ie->ie_flags & IE_SOFT) == 0 &&
		    TAILQ_FIRST(&ie->ie_handlers) != NULL)
			break;
	mtx_unlock(&event_lock);
	return (ie);
}

int
intr_setaffinity(int irq, void *m)
{
#ifdef SUKWON
	struct intr_event *ie;
	cpuset_t *mask;
	u_char cpu;
	int n;

	mask = m;
	cpu = NOCPU;
	/*
	 * If we're setting all cpus we can unbind.  Otherwise make sure
	 * only one cpu is in the set.
	 */
	if (CPU_CMP(cpuset_root, mask)) {
		for (n = 0; n < CPU_SETSIZE; n++) {
			if (!CPU_ISSET(n, mask))
				continue;
			if (cpu != NOCPU)
				return (EINVAL);
			cpu = (u_char)n;
		}
	}
	ie = intr_lookup(irq);
	if (ie == NULL)
		return (ESRCH);
	return (intr_event_bind(ie, cpu));
#else
  return EOPNOTSUPP;
#endif
}

int
intr_getaffinity(int irq, void *m)
{
#ifdef SUKWON
	struct intr_event *ie;
	cpuset_t *mask;

	mask = m;
	ie = intr_lookup(irq);
	if (ie == NULL)
		return (ESRCH);
	CPU_ZERO(mask);
	mtx_lock(&ie->ie_lock);
	if (ie->ie_cpu == NOCPU)
		CPU_COPY(cpuset_root, mask);
	else
		CPU_SET(ie->ie_cpu, mask);
	mtx_unlock(&ie->ie_lock);
	return (0);
#else
  return 0;
#endif
}

int
intr_event_destroy(struct intr_event *ie)
{

	mtx_lock(&event_lock);
	mtx_lock(&ie->ie_lock);
	if (!TAILQ_EMPTY(&ie->ie_handlers)) {
		mtx_unlock(&ie->ie_lock);
		mtx_unlock(&event_lock);
		return (EBUSY);
	}
	TAILQ_REMOVE(&event_list, ie, ie_list);
#ifndef notyet
	if (ie->ie_thread != NULL) {
		ithread_destroy(ie->ie_thread);
		ie->ie_thread = NULL;
	}
#endif
	mtx_unlock(&ie->ie_lock);
	mtx_unlock(&event_lock);
	mtx_destroy(&ie->ie_lock);
	free(ie, M_ITHREAD);
	return (0);
}

static struct intr_thread *
ithread_create(const char *name)
{
	struct intr_thread *ithd;
	void *td;
#ifdef SUKWON
	int error;
#endif

	ithd = malloc(sizeof(struct intr_thread), M_ITHREAD, M_WAITOK | M_ZERO);

#ifdef SUKWON
	error = kproc_kthread_add(ithread_loop, ithd, &intrproc,
		    &td, RFSTOPPED | RFHIGHPID,
	    	    0, "intr", "%s", name);
	if (error)
		panic("kproc_create() failed with %d", error);
	thread_lock(td);
	sched_class(td, PRI_ITHD);
	TD_SET_IWAIT(td);
	thread_unlock(td);
	td->td_pflags |= TDP_ITHREAD;
#else
  td = KOS_ThreadCreate(ithread_loop, ithd, RFSTOPPED | RFHIGHPID, "%s", name);
  thread_lock(td);
  TD_SET_IWAIT(td);
  thread_unlock(td);
#endif
  ithd->it_thread = td;
	return (ithd);
}

static void
ithread_destroy(struct intr_thread *ithread)
{
	void *td;

	td = ithread->it_thread;
	thread_lock(td);
	ithread->it_flags |= IT_DEAD;
	if (TD_AWAITING_INTR(td)) {
		TD_CLR_IWAIT(td);
#ifdef SUKWON
		sched_add(td, SRQ_INTR);
#else
    KOS_ScheduleLocked(td, 0);
#endif
	}
	thread_unlock(td);
}

int
intr_event_add_handler(struct intr_event *ie, const char *name,
    driver_filter_t filter, driver_intr_t handler, void *arg, u_char pri,
    enum intr_type flags, void **cookiep)
{
	struct intr_handler *ih, *temp_ih;
	struct intr_thread *it;

	if (ie == NULL || name == NULL || (handler == NULL && filter == NULL))
		return (EINVAL);

	/* Allocate and populate an interrupt handler structure. */
	ih = malloc(sizeof(struct intr_handler), M_ITHREAD, M_WAITOK | M_ZERO);
	ih->ih_filter = filter;
	ih->ih_handler = handler;
	ih->ih_argument = arg;
	strlcpy(ih->ih_name, name, sizeof(ih->ih_name));
	ih->ih_event = ie;
	ih->ih_pri = pri;
	if (flags & INTR_EXCL)
		ih->ih_flags = IH_EXCLUSIVE;
	if (flags & INTR_MPSAFE)
		ih->ih_flags |= IH_MPSAFE;
	if (flags & INTR_ENTROPY)
		ih->ih_flags |= IH_ENTROPY;

	/* We can only have one exclusive handler in a event. */
	mtx_lock(&ie->ie_lock);
	if (!TAILQ_EMPTY(&ie->ie_handlers)) {
		if ((flags & INTR_EXCL) ||
		    (TAILQ_FIRST(&ie->ie_handlers)->ih_flags & IH_EXCLUSIVE)) {
			mtx_unlock(&ie->ie_lock);
			free(ih, M_ITHREAD);
			return (EINVAL);
		}
	}

	/* Create a thread if we need one. */
	while (ie->ie_thread == NULL && handler != NULL) {
		if (ie->ie_flags & IE_ADDING_THREAD)
			msleep(ie, &ie->ie_lock, 0, "ithread", 0);
		else {
			ie->ie_flags |= IE_ADDING_THREAD;
			mtx_unlock(&ie->ie_lock);
			it = ithread_create("intr: newborn");
			mtx_lock(&ie->ie_lock);
			ie->ie_flags &= ~IE_ADDING_THREAD;
			ie->ie_thread = it;
			it->it_event = ie;
			ithread_update(it);
			wakeup(ie);
		}
	}

	/* Add the new handler to the event in priority order. */
	TAILQ_FOREACH(temp_ih, &ie->ie_handlers, ih_next) {
		if (temp_ih->ih_pri > ih->ih_pri)
			break;
	}
	if (temp_ih == NULL)
		TAILQ_INSERT_TAIL(&ie->ie_handlers, ih, ih_next);
	else
		TAILQ_INSERT_BEFORE(temp_ih, ih, ih_next);
	intr_event_update(ie);

	mtx_unlock(&ie->ie_lock);

	if (cookiep != NULL)
		*cookiep = ih;
	return (0);
}

/*
 * Append a description preceded by a ':' to the name of the specified
 * interrupt handler.
 */
int
intr_event_describe_handler(struct intr_event *ie, void *cookie,
    const char *descr)
{
	struct intr_handler *ih;
	size_t space;
	char *start;

	mtx_lock(&ie->ie_lock);
	ih = cookie;

	/*
	 * Look for an existing description by checking for an
	 * existing ":".  This assumes device names do not include
	 * colons.  If one is found, prepare to insert the new
	 * description at that point.  If one is not found, find the
	 * end of the name to use as the insertion point.
	 */
	start = index(ih->ih_name, ':');
	if (start == NULL)
		start = index(ih->ih_name, 0);

	/*
	 * See if there is enough remaining room in the string for the
	 * description + ":".  The "- 1" leaves room for the trailing
	 * '\0'.  The "+ 1" accounts for the colon.
	 */
	space = sizeof(ih->ih_name) - (start - ih->ih_name) - 1;
	if (strlen(descr) + 1 > space) {
		mtx_unlock(&ie->ie_lock);
		return (ENOSPC);
	}

	/* Append a colon followed by the description. */
	*start = ':';
	strcpy(start + 1, descr);
	intr_event_update(ie);
	mtx_unlock(&ie->ie_lock);
	return (0);
}

/*
 * Return the ie_source field from the intr_event an intr_handler is
 * associated with.
 */
void *
intr_handler_source(void *cookie)
{
	struct intr_handler *ih;
	struct intr_event *ie;

	ih = (struct intr_handler *)cookie;
	if (ih == NULL)
		return (NULL);
	ie = ih->ih_event;
	KASSERT(ie != NULL,
	    ("interrupt handler \"%s\" has a NULL interrupt event",
	    ih->ih_name));
	return (ie->ie_source);
}

/*
 * Sleep until an ithread finishes executing an interrupt handler.
 *
 * XXX Doesn't currently handle interrupt filters or fast interrupt
 * handlers.  This is intended for compatibility with linux drivers
 * only.  Do not use in BSD code.
 */
void
_intr_drain(int irq)
{
	struct intr_event *ie;
	struct intr_thread *ithd;
	void *td;

	ie = intr_lookup(irq);
	if (ie == NULL)
		return;
	if (ie->ie_thread == NULL)
		return;
	ithd = ie->ie_thread;
	td = ithd->it_thread;
	/*
	 * We set the flag and wait for it to be cleared to avoid
	 * long delays with potentially busy interrupt handlers
	 * were we to only sample TD_AWAITING_INTR() every tick.
	 */
	thread_lock(td);
	if (!TD_AWAITING_INTR(td)) {
		ithd->it_flags |= IT_WAIT;
		while (ithd->it_flags & IT_WAIT) {
			thread_unlock(td);
			pause("idrain", 1);
			thread_lock(td);
		}
	}
	thread_unlock(td);
	return;
}


int
intr_event_remove_handler(void *cookie)
{
	struct intr_handler *handler = (struct intr_handler *)cookie;
	struct intr_event *ie;

	if (handler == NULL)
		return (EINVAL);
	ie = handler->ih_event;
	KASSERT(ie != NULL,
	    ("interrupt handler \"%s\" has a NULL interrupt event",
	    handler->ih_name));
	mtx_lock(&ie->ie_lock);
	/*
	 * If there is no ithread, then just remove the handler and return.
	 * XXX: Note that an INTR_FAST handler might be running on another
	 * CPU!
	 */
	if (ie->ie_thread == NULL) {
		TAILQ_REMOVE(&ie->ie_handlers, handler, ih_next);
		mtx_unlock(&ie->ie_lock);
		free(handler, M_ITHREAD);
		return (0);
	}

	/*
	 * If the interrupt thread is already running, then just mark this
	 * handler as being dead and let the ithread do the actual removal.
	 *
	 * During a cold boot while cold is set, msleep() does not sleep,
	 * so we have to remove the handler here rather than letting the
	 * thread do it.
	 */
	thread_lock(ie->ie_thread->it_thread);
	if (!TD_AWAITING_INTR(ie->ie_thread->it_thread) && !cold) {
		handler->ih_flags |= IH_DEAD;

		/*
		 * Ensure that the thread will process the handler list
		 * again and remove this handler if it has already passed
		 * it on the list.
		 */
		ie->ie_thread->it_need = 1;
	} else
		TAILQ_REMOVE(&ie->ie_handlers, handler, ih_next);
	thread_unlock(ie->ie_thread->it_thread);
	while (handler->ih_flags & IH_DEAD)
		msleep(handler, &ie->ie_lock, 0, "iev_rmh", 0);
	intr_event_update(ie);
	mtx_unlock(&ie->ie_lock);
	free(handler, M_ITHREAD);
	return (0);
}

static int
intr_event_schedule_thread(struct intr_event *ie)
{
	struct intr_thread *it;
	void *td;

	/*
	 * If no ithread or no handlers, then we have a stray interrupt.
	 */
	if (ie == NULL || TAILQ_EMPTY(&ie->ie_handlers) ||
	    ie->ie_thread == NULL)
		return (EINVAL);

	it = ie->ie_thread;
	td = it->it_thread;

	/*
	 * Set it_need to tell the thread to keep running if it is already
	 * running.  Then, lock the thread and see if we actually need to
	 * put it on the runqueue.
	 */
	it->it_need = 1;
	thread_lock(td);
	if (TD_AWAITING_INTR(td)) {
		TD_CLR_IWAIT(td);
#ifdef SUKWON
		sched_add(td, SRQ_INTR);
#else
    KOS_ScheduleLocked(td, 0);
#endif
	}
	thread_unlock(td);

	return (0);
}

/*
 * Allow interrupt event binding for software interrupt handlers -- a no-op,
 * since interrupts are generated in software rather than being directed by
 * a PIC.
 */
static int
swi_assign_cpu(void *arg, u_char cpu)
{

	return (0);
}

/*
 * Add a software interrupt handler to a specified event.  If a given event
 * is not specified, then a new event is created.
 */
int
swi_add(struct intr_event **eventp, const char *name, driver_intr_t handler,
	    void *arg, int pri, enum intr_type flags, void **cookiep)
{
#ifdef SUKWON
	void *td;
#endif
	struct intr_event *ie;
	int error;

	if (flags & INTR_ENTROPY)
		return (EINVAL);

	ie = (eventp != NULL) ? *eventp : NULL;

	if (ie != NULL) {
		if (!(ie->ie_flags & IE_SOFT))
			return (EINVAL);
	} else {
		error = intr_event_create(&ie, NULL, IE_SOFT, 0,
		    NULL, NULL, NULL, swi_assign_cpu, "swi%d:", pri);
		if (error)
			return (error);
		if (eventp != NULL)
			*eventp = ie;
	}
	error = intr_event_add_handler(ie, name, NULL, handler, arg,
	    PI_SWI(pri), flags, cookiep);
	if (error)
		return (error);
#ifdef SUKWON
	if (pri == SWI_CLOCK) {
		td = ie->ie_thread->it_thread;
		thread_lock(td);
		td->td_flags |= TDF_NOLOAD;
		thread_unlock(td);
	}
#endif
	return (0);
}

/*
 * Schedule a software interrupt thread.
 */
void
swi_sched(void *cookie, int flags)
{
	struct intr_handler *ih = (struct intr_handler *)cookie;
	struct intr_event *ie = ih->ih_event;
	int error;

	/*
	 * Set ih_need for this handler so that if the ithread is already
	 * running it will execute this handler on the next pass.  Otherwise,
	 * it will execute it the next time it runs.
	 */
	atomic_store_rel_int(&ih->ih_need, 1);

	if (!(flags & SWI_DELAY)) {
		error = intr_event_schedule_thread(ie);
		KASSERT(error == 0, ("stray software interrupt"));
	}
}

/*
 * Remove a software interrupt handler.  Currently this code does not
 * remove the associated interrupt event if it becomes empty.  Calling code
 * may do so manually via intr_event_destroy(), but that's not really
 * an optimal interface.
 */
int
swi_remove(void *cookie)
{

	return (intr_event_remove_handler(cookie));
}

/*
 * This is a public function for use by drivers that mux interrupt
 * handlers for child devices from their interrupt handler.
 */
void
intr_event_execute_handlers(struct intr_event *ie)
{
	struct intr_handler *ih, *ihn;

	TAILQ_FOREACH_SAFE(ih, &ie->ie_handlers, ih_next, ihn) {
		/*
		 * If this handler is marked for death, remove it from
		 * the list of handlers and wake up the sleeper.
		 */
		if (ih->ih_flags & IH_DEAD) {
			mtx_lock(&ie->ie_lock);
			TAILQ_REMOVE(&ie->ie_handlers, ih, ih_next);
			ih->ih_flags &= ~IH_DEAD;
			wakeup(ih);
			mtx_unlock(&ie->ie_lock);
			continue;
		}

		/* Skip filter only handlers */
		if (ih->ih_handler == NULL)
			continue;

		/*
		 * For software interrupt threads, we only execute
		 * handlers that have their need flag set.  Hardware
		 * interrupt threads always invoke all of their handlers.
		 */
		if (ie->ie_flags & IE_SOFT) {
			if (!ih->ih_need)
				continue;
			else
				atomic_store_rel_int(&ih->ih_need, 0);
		}

		/* Execute this handler. */
#ifdef SUKWON
		if (!(ih->ih_flags & IH_MPSAFE))
			mtx_lock(&Giant);
#endif
		ih->ih_handler(ih->ih_argument);
#ifdef SUKWON
		if (!(ih->ih_flags & IH_MPSAFE))
			mtx_unlock(&Giant);
#endif
	}
}

static void
ithread_execute_handlers(struct intr_event *ie)
{

	/* Interrupt handlers should not sleep. */
#ifdef SUKWON
	if (!(ie->ie_flags & IE_SOFT))
		THREAD_NO_SLEEPING();
#endif
	intr_event_execute_handlers(ie);
#ifdef SUKWON
	if (!(ie->ie_flags & IE_SOFT))
		THREAD_SLEEPING_OK();
#endif

#ifdef SUKWON
	/*
	 * Interrupt storm handling:
	 *
	 * If this interrupt source is currently storming, then throttle
	 * it to only fire the handler once  per clock tick.
	 *
	 * If this interrupt source is not currently storming, but the
	 * number of back to back interrupts exceeds the storm threshold,
	 * then enter storming mode.
	 */
	if (intr_storm_threshold != 0 && ie->ie_count >= intr_storm_threshold &&
	    !(ie->ie_flags & IE_SOFT)) {
		/* Report the message only once every second. */
		if (ppsratecheck(&ie->ie_warntm, &ie->ie_warncnt, 1)) {
			printf(
	"interrupt storm detected on \"%s\"; throttling interrupt source\n",
			    ie->ie_name);
		}
		pause("istorm", 1);
	} else
#endif
		ie->ie_count++;

	/*
	 * Now that all the handlers have had a chance to run, reenable
	 * the interrupt source.
	 */
	if (ie->ie_post_ithread != NULL)
		ie->ie_post_ithread(ie->ie_source);
}

/*
 * This is the main code for interrupt threads.
 */
static void
ithread_loop(void *arg)
{
	struct intr_thread *ithd;
	struct intr_event *ie;
	void *td;
	int wake;

#ifdef SUKWON
	td = curthread;
#else
  td = KOS_CurThread();
#endif
	ithd = (struct intr_thread *)arg;
	KASSERT(ithd->it_thread == td,
	    ("%s: ithread and proc linkage out of sync", __func__));
	ie = ithd->it_event;
	ie->ie_count = 0;
	wake = 0;

	/*
	 * As long as we have interrupts outstanding, go through the
	 * list of handlers, giving each one a go at it.
	 */
	for (;;) {
		/*
		 * If we are an orphaned thread, then just die.
		 */
		if (ithd->it_flags & IT_DEAD) {
			free(ithd, M_ITHREAD);
#ifdef SUKWON
			kthread_exit();
#else
      break;
#endif
		}

		/*
		 * Service interrupts.  If another interrupt arrives while
		 * we are running, it will set it_need to note that we
		 * should make another pass.
		 */
		while (ithd->it_need) {
			/*
			 * This might need a full read and write barrier
			 * to make sure that this write posts before any
			 * of the memory or device accesses in the
			 * handlers.
			 */
			atomic_store_rel_int(&ithd->it_need, 0);
			ithread_execute_handlers(ie);
		}
		mtx_assert(&Giant, MA_NOTOWNED);

		/*
		 * Processed all our interrupts.  Now get the sched
		 * lock.  This may take a while and it_need may get
		 * set again, so we have to check it again.
		 */
		thread_lock(td);
		if (!ithd->it_need && !(ithd->it_flags & (IT_DEAD | IT_WAIT))) {
			TD_SET_IWAIT(td);
			ie->ie_count = 0;
#ifdef SUKWON
			mi_switch(SW_VOL | SWT_IWAIT, NULL);
#else
      KOS_Suspend(td);
#endif
		}
		if (ithd->it_flags & IT_WAIT) {
			wake = 1;
			ithd->it_flags &= ~IT_WAIT;
		}
		thread_unlock(td);
		if (wake) {
			wakeup(ithd);
			wake = 0;
		}
	}
}

/*
 * Main interrupt handling body.
 *
 * Input:
 * o ie:                        the event connected to this interrupt.
 * o frame:                     some archs (i.e. i386) pass a frame to some.
 *                              handlers as their main argument.
 * Return value:
 * o 0:                         everything ok.
 * o EINVAL:                    stray interrupt.
 */
int
intr_event_handle(struct intr_event *ie, struct trapframe *frame)
{
	struct intr_handler *ih;
	struct trapframe *oldframe;
	void *td;
	int error, ret, thread;

#ifdef SUKWON
	td = curthread;
#else
  td = KOS_CurThread();
#endif

	/* An interrupt with no event or handlers is a stray interrupt. */
	if (ie == NULL || TAILQ_EMPTY(&ie->ie_handlers))
		return (EINVAL);

	/*
	 * Execute fast interrupt handlers directly.
	 * To support clock handlers, if a handler registers
	 * with a NULL argument, then we pass it a pointer to
	 * a trapframe as its argument.
	 */
#ifdef SUKWON
	td->td_intr_nesting_level++;
#endif
	thread = 0;
	ret = 0;
	critical_enter();
#ifdef SUKWON
	oldframe = td->td_intr_frame;
	td->td_intr_frame = frame;
#endif
	TAILQ_FOREACH(ih, &ie->ie_handlers, ih_next) {
		if (ih->ih_filter == NULL) {
			thread = 1;
			continue;
		}
		if (ih->ih_argument == NULL)
			ret = ih->ih_filter(frame);
		else
			ret = ih->ih_filter(ih->ih_argument);
		KASSERT(ret == FILTER_STRAY ||
		    ((ret & (FILTER_SCHEDULE_THREAD | FILTER_HANDLED)) != 0 &&
		    (ret & ~(FILTER_SCHEDULE_THREAD | FILTER_HANDLED)) == 0),
		    ("%s: incorrect return value %#x from %s", __func__, ret,
		    ih->ih_name));

		/* 
		 * Wrapper handler special handling:
		 *
		 * in some particular cases (like pccard and pccbb), 
		 * the _real_ device handler is wrapped in a couple of
		 * functions - a filter wrapper and an ithread wrapper.
		 * In this case (and just in this case), the filter wrapper 
		 * could ask the system to schedule the ithread and mask
		 * the interrupt source if the wrapped handler is composed
		 * of just an ithread handler.
		 *
		 * TODO: write a generic wrapper to avoid people rolling 
		 * their own
		 */
		if (!thread) {
			if (ret == FILTER_SCHEDULE_THREAD)
				thread = 1;
		}
	}
#ifdef SUKWON
	td->td_intr_frame = oldframe;
#endif

	if (thread) {
		if (ie->ie_pre_ithread != NULL)
			ie->ie_pre_ithread(ie->ie_source);
	} else {
		if (ie->ie_post_filter != NULL)
			ie->ie_post_filter(ie->ie_source);
	}
	
	/* Schedule the ithread if needed. */
	if (thread) {
		error = intr_event_schedule_thread(ie);
		KASSERT(error == 0, ("bad stray interrupt"));
	}
	critical_exit();
#ifdef SUKWON
	td->td_intr_nesting_level--;
#endif
	return (0);
}

#ifdef SUKWON
/*
 * Start standard software interrupt threads
 */
static void
start_softintr(void *dummy)
{

	if (swi_add(NULL, "vm", swi_vm, NULL, SWI_VM, INTR_MPSAFE, &vm_ih))
		panic("died while creating vm swi ithread");
}
SYSINIT(start_softintr, SI_SUB_SOFTINTR, SI_ORDER_FIRST, start_softintr,
    NULL);
#endif
