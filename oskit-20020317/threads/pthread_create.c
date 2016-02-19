/*
 * Copyright (c) 1996-2001 University of Utah and the Flux Group.
 * All rights reserved.
 * 
 * This file is part of the Flux OSKit.  The OSKit is free software, also known
 * as "open source;" you can redistribute it and/or modify it under the terms
 * of the GNU General Public License (GPL), version 2, as published by the Free
 * Software Foundation (FSF).  To explore alternate licensing terms, contact
 * the University of Utah at csl-dist@cs.utah.edu or +1-801-585-3271.
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */

/*
 * Create a new thread and return a thread ID.
 */
#include <threads/pthread_internal.h>
#include <threads/pthread_ipc.h>
#include <oskit/machine/base_stack.h>
#include <oskit/machine/base_paging.h>
#include <oskit/c/sys/mman.h>
#include <strings.h>
#include <string.h>
#ifdef HIGHRES_THREADTIMES
#include "machine/hirestime.h"
#endif

/*
 * Hardwired.
 */
#define DEFAULT_STACK_SIZE		(3 * PAGE_SIZE)

/*
 * Need a lock to protect the tidtothread array.
 */
pthread_lock_t	pthread_create_lock = PTHREAD_LOCK_INITIALIZER;

/*
 * Strictly for GDB macros.
 */
int		threads_maxtid = THREADS_MAX_THREAD;

/*
 * Forward decl.
 */
void		pthread_prepare_timer(pthread_thread_t *pthread);

extern int	threads_alive;

/* 
 * Thread create hook.
 */
static oskit_pthread_create_hook_t	threads_create_hook;

/*
 * Create a thread at the given priority. The thread is started as
 * function(arg). The caller can specify a stacksize, or if 0 supply
 * a reasonable default.
 */
int
pthread_create(pthread_t *tid, const pthread_attr_t *attr,
	       void *(*function)(void *), void *argument)
{
	pthread_thread_t	*pthread;
	int			i;
#ifdef  CPU_INHERIT
	pthread_thread_t	*pscheduler;
#endif

	if (! attr)
		attr = &pthread_attr_default;

	pthread = pthread_create_internal(function, argument, attr);

	if (attr->detachstate == PTHREAD_CREATE_DETACHED)
		pthread->flags |= THREAD_DETACHED;

	/*
	 * Signal mask is inherited from creating thread.
	 */
	pthread->sigmask = CURPTHREAD()->sigmask;

#ifdef  CPU_INHERIT
	pscheduler = CURPTHREAD()->scheduler;

	if (attr->scheduler) {
		if (tidtothread(attr->scheduler) == NULL_THREADPTR)
			printf("pthread_create: Bad scheduler: tid(%p)\n",
			       attr->scheduler);
		else
			pscheduler = tidtothread(attr->scheduler);
	}

	pthread->scheduler  = pscheduler;
#endif
	/*
	 * Find a free slot in tidtothread.
	 */
	assert_preemption_enabled();
	disable_preemption();
	pthread_lock(&pthread_create_lock);
	for (i = 1; i < THREADS_MAX_THREAD; i++)
		if (threads_tidtothread[i] == 0)
			break;

	if (i == THREADS_MAX_THREAD)
		panic("threads_create: Too many threads!\n");

	threads_tidtothread[i] = pthread;
	pthread->tid = (pthread_t) i;
	pthread_unlock(&pthread_create_lock);

	/* Call the thread create hook. */
	if (threads_create_hook) {
	    (*threads_create_hook)(pthread->tid);
	}

#ifdef  CPU_INHERIT
	/*
	 * Send a new thread message to the scheduler.
	 */
	if (pthread->scheduler) {
		schedmsg_t	msg;

		msg.type    = MSG_SCHED_NEWTHREAD;
		msg.tid     = pthread->tid;
		msg.opaque  = attr->priority;
		msg.opaque2 = attr->policy;
		pthread_sched_message_send(pthread->scheduler, &msg);
	}
#else
	/*
	 * Go ahead and schedule the thread.
	 */
	pthread_sched_setrunnable(pthread);
#endif

	*tid = (pthread_t *) pthread->tid;
	threads_alive++;

	enable_preemption();
	return 0;
}

#ifndef RTAI_SCHEDULER
pthread_thread_t *
pthread_create_internal(void *(*function)(void *), void *argument,
			const pthread_attr_t *attr)
{
	pthread_thread_t	*pthread;

	if (! attr)
		attr = &pthread_attr_default;

	if ((pthread =
	     (pthread_thread_t *) threads_allocator(PAGE_SIZE)) == NULL)
		panic("pthread_create_internal: Not enough memory");

	memset((void *) pthread, 0, PAGE_SIZE);

	if ((pthread->ssize = attr->stacksize) < PTHREAD_STACK_MIN)
		return NULL_THREADPTR;

	/*
	 * Look for user provided stack. If one is provided, the stack
	 * size better be right!
	 */
	if (attr->stackaddr) {
		pthread->pstk  = attr->stackaddr;
		pthread->flags = THREAD_USERSTACK;
	}
	else {
		if (attr->guardsize) {
			pthread->guardsize = round_page(attr->guardsize);
			pthread->ssize    += pthread->guardsize;
		}

		if ((pthread->pstk = threads_allocator(pthread->ssize)) == NULL)
			panic("pthread_create_internal: Not enough memory");
	}
	memset(pthread->pstk, 0, pthread->ssize);

	/*
	 * Set up the thread.
	 */
	pthread->ppcb     = &pthread->pcb;
	pthread->cookie   = argument;
	pthread->func     = function;
	pthread->cancelstate = PTHREAD_CANCEL_ENABLE;
	pthread->canceltype  = PTHREAD_CANCEL_DEFERRED;
#ifdef	PRI_INHERIT
	queue_init(&pthread->waiters);
#endif
#ifdef	CPU_INHERIT
	queue_init(&pthread->donors);
	pthread->schedflags = SCHED_READY;
#endif
	queue_init(&pthread->ipc_state.senders);
	pthread_lock_init(&pthread->lock);
	pthread_lock_init(&pthread->schedlock);
	pthread_lock_init(&pthread->waitlock);
	pthread_lock_init(&pthread->siglock);
	pthread_mutex_init(&pthread->mutex, NULL);
	pthread_cond_init(&pthread->cond, NULL);
	pthread_prepare_timer(pthread);

#ifdef	THREAD_STATS
	pthread->stats.rmin = pthread->stats.qmin = 100000000; /* XXX */
#endif

	/*
	 * All "internal" threads block all signals. pthread_create above
	 * will reset the signal mask to the correct value.
	 */
	sigfillset(&pthread->sigmask);

#ifdef  DEFAULT_SCHEDULER
	/*
	 * Setup the scheduler stuff. Lets just pass this off to the
	 * scheduling module to avoid duplicate code here.
	 */
	{
		struct sched_param	param;

#ifdef  PTHREAD_SCHED_POSIX
		if (SCHED_POLICY_POSIX(attr->policy)) {
			param.priority = attr->priority;
		}
#endif
#ifdef  PTHREAD_REALTIME
		if (SCHED_POLICY_REALTIME(attr->policy)) {
			param.start    = attr->start;
			param.deadline = attr->deadline;
			param.period   = attr->period;
		}
#endif
#ifdef  PTHREAD_SCHED_STRIDE
		if (attr->policy == SCHED_STRIDE)
			param.tickets  = attr->tickets;
#endif
		pthread_sched_init_schedstate(pthread, attr->policy, &param);
	}
#endif  /* DEFAULT_SCHEDULER */

	/*
	 * Most of the work is done in the machine dependent code.
	 */
	thread_setup(pthread);

	return pthread;
}
#endif


/*
 * Install the thread create hook.  Only one hook can be set.
 * Returns previous hook function.
 */
oskit_pthread_create_hook_t
oskit_pthread_create_hook_set(oskit_pthread_create_hook_t hook)
{
    oskit_pthread_create_hook_t ohook = threads_create_hook;
    threads_create_hook = hook;
    return ohook;
}


/*
 * Create a stub thread for the main process.
 */
pthread_thread_t *
pthread_init_mainthread(pthread_thread_t *pthread)
{
	pthread->flags    = 0;
	pthread->ppcb     = &pthread->pcb;
	pthread->pstk     = base_stack_start;
	pthread->ssize    = base_stack_size;
	pthread->cookie   = 0;
	pthread->func     = 0;
	pthread->cancelstate = PTHREAD_CANCEL_ENABLE;
	pthread->canceltype  = PTHREAD_CANCEL_DEFERRED;
#ifdef	PRI_INHERIT
	queue_init(&pthread->waiters);
#endif
#ifdef	CPU_INHERIT
	queue_init(&pthread->donors);
	pthread->schedflags = SCHED_READY;
#endif
	queue_init(&pthread->ipc_state.senders);
	pthread_lock_init(&pthread->lock);
	pthread_lock_init(&pthread->schedlock);
	pthread_lock_init(&pthread->waitlock);
	pthread_lock_init(&pthread->siglock);
	pthread_mutex_init(&pthread->mutex, NULL);
	pthread_cond_init(&pthread->cond, NULL);
	pthread_prepare_timer(pthread);
#ifdef	STACKGUARD
	pthread_mprotect((oskit_addr_t) pthread->pstk, PAGE_SIZE, PROT_READ);
#endif
#ifdef  RTAI_SCHEDULER
	rtai_pthread_create_mainthread(pthread, &pthread->rtai_priv,
				       pthread->priority);
#endif
#ifdef	HIGHRES_THREADTIMES
	pthread_hirestime_get(&pthread->rstamp);
#endif
#ifdef	THREAD_STATS
	pthread->stats.rstamp = STAT_STAMPGET();
	pthread->stats.rmin = pthread->stats.qmin = 100000000; /* XXX */
#endif
#ifdef  DEFAULT_SCHEDULER
	/*
	 * Quite silly. Need a better way to determine who the initial
	 * scheduler for the main thread should be. Not that it matters
	 * too much, since the program can easily change it whenever
	 * it likes.
	 */
	{
		struct sched_param	param;
		int			policy;
		
#ifdef  PTHREAD_SCHED_POSIX
		param.priority = PRIORITY_NORMAL;
		policy	       = SCHED_RR;
#elif   defined(PTHREAD_SCHED_EDF)
		param.start    = { 0, 0 };
		param.deadline = { 0, 0 };
		param.period   = { 0, 30000000 };
		policy         = SCHED_EDF;
#elif   defined(PTHREAD_SCHED_STRIDE)
		param.tickets  = 100;
		policy         = SCHED_STRIDE;
#else
#error	Define main scheduler
#endif
		pthread_sched_init_schedstate(pthread, policy, &param);
	}
#endif
	/*
	 * Reserve slot 0 for root scheduler when CPU_INHERIT defined.
	 */
	threads_tidtothread[1] = pthread;
	pthread->tid = (pthread_t) 1;

	return pthread;
}

/*
 * Timer stuff. Create the timers for sleep and cond_timedwait.
 * Do this at init time to avoid calls to malloc later.
 *
 * I could probably combine these and dispatch on wait state.
 */
oskit_error_t	pthread_condwait_timeout(struct oskit_iunknown *l, void *arg);
oskit_error_t	pthread_sleep_timeout(struct oskit_iunknown *l, void *arg);

void
pthread_prepare_timer(pthread_thread_t *pthread)
{
	extern oskit_clock_t    *oskit_system_clock;    /* XXX */
	oskit_listener_t	*listener;

	/* XXX invokes "malloc" */
	listener = oskit_create_listener(pthread_condwait_timeout, pthread);

	oskit_clock_createtimer(oskit_system_clock, &pthread->condtimer);
	oskit_timer_setlistener(pthread->condtimer, listener);
	oskit_listener_release(listener);

	/* XXX invokes "malloc" */
	listener = oskit_create_listener(pthread_sleep_timeout, pthread);

	oskit_clock_createtimer(oskit_system_clock, &pthread->sleeptimer);
	oskit_timer_setlistener(pthread->sleeptimer, listener);
	oskit_listener_release(listener);
}
