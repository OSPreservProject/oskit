/*
 * Copyright (c) 1996, 1998-2001 University of Utah and the Flux Group.
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

#ifdef	DEFAULT_SCHEDULER

/*
 * Default POSIX scheduler.
 */
#include <threads/pthread_internal.h>
#include "pthread_schedconf.h"
#include "pthread_signal.h"
#include "sched_realtime.h"
#ifdef HIGHRES_THREADTIMES
#include "machine/hirestime.h"
#endif

/*
 * The scheduler lock is used elsewhere. It must always be taken with
 * interrupts disabled since it is used within interrupt handlers to
 * change the run queue.
 */
pthread_lock_t		pthread_sched_lock    = PTHREAD_LOCK_INITIALIZER;

#ifdef	SCHED_STATS
static struct pthread_gstats stats;
void	dump_scheduler_stats();
#endif

#ifdef	THREAD_STATS
static struct pthread_history pthread_stat_history[PTHREAD_HISTORY_SIZE];
static struct pthread_history *php = pthread_stat_history;
void	dump_thread_stats();
#endif

static oskit_pthread_csw_hook_t aftercsw_hook;

/*
 * Initialize whatever schedulers are linked in.
 */
void
pthread_init_scheduler(void)
{
	int	i, okay = 0;

	for (i = 0; i < nschedulers; i++) {
		if (scheduler_array[i].init) {
			scheduler_array[i].init();
			okay++;
		}
	}
	if (!okay)
		panic(__FUNCTION__ ": No schedulers linked in!");
#ifdef	SCHED_STATS
	atexit(dump_scheduler_stats);
#endif
#ifdef	THREAD_STATS
	atexit(dump_thread_stats);
#endif
}

/*
 * This handles the details of finding another thread to switch to.
 * Return value to indicate whether a switch actually happened.
 */
static inline int
SCHED_DISPATCH(resched_flags_t reason, pthread_thread_t *pthread)
{
	assert(pthread->scheduler);

	return pthread->scheduler->dispatch(reason, pthread);
}

int
pthread_sched_dispatch(resched_flags_t reason, pthread_thread_t *pnext)
{
	pthread_thread_t	*pthread = CURPTHREAD();
	int			rerun, preemptable;
#ifdef  SCHED_STATS
	stat_stamp_t		before;
#endif

	save_preemption_enable(preemptable);
	assert_interrupts_disabled();

#ifdef  SCHED_STATS
	before = STAT_STAMPGET();
#endif
	/*
	 * Going to muck with the scheduler queues, so take that lock.
	 */
	pthread_lock(&pthread_sched_lock);

	/*
	 * Decide what to do with the current thread.
	 */
	rerun = SCHED_DISPATCH(reason, pthread);
	
	/*
	 * Clear preemption flag
	 */
	pthread->preempt = PREEMPT_NEEDED = 0;

	/*
	 * See if we need to keep running the same thread.
	 */
	if (rerun) {
		if (pnext) {
#ifdef	THREAD_STATS
			pnext->stats.qstamp = STAT_STAMPGET();
#endif
			pthread_unlock(&pnext->lock);
			rerun = SCHED_DISPATCH(RESCHED_YIELD, pnext);
			assert(!rerun);
		}

#ifdef	THREAD_STATS
		pthread->stats.rescheds++;
#endif
		pthread_unlock(&pthread_sched_lock);
		pthread_unlock(&pthread->schedlock);

		/*
		 * Deliver pending signals as long as we have the
		 * the opportunity to do so.
		 */
		SIGCHECK(pthread);
		return 0;
	}

	/*
	 * Now find a thread to run.
	 */
	if (pnext) {
		/*
		 * Locked thread was provided, so done with the scheduler.
		 */
		pthread_unlock(&pthread_sched_lock);
	}
	else {
		int	i;

		/*
		 * Ask all schedulers in order. Looks bad, but not really.
		 */
		for (i = 0; i < nschedulers; i++) {
			if (scheduler_array[i].thread_next &&
			    (pnext = scheduler_array[i].thread_next()))
				break;
		}
		
		/*
		 * Nothing to run, so idle.
		 */
		if (!pnext) {
#ifdef  THREADS_DEBUG
			pthread_lock(&threads_sleepers_lock);
			if (! threads_sleepers)
				panic("RESCHED: "
				      "Nothing to run. Might be deadlock!");
			pthread_unlock(&threads_sleepers_lock);
#endif
			pnext = IDLETHREAD;
		}
		
		pthread_unlock(&pthread_sched_lock);

		/*
		 * Avoid switch into same thread. 
		 */
		if (pnext == pthread) {
#ifdef  SCHED_STATS
			stats.switchessame++;
			stats.switchsame_cycles += STAT_STAMPDIFF(before);
#endif
#ifdef	THREAD_STATS
			pthread->stats.rescheds++;
#endif
			pthread_unlock(&pthread->schedlock);

			/*
			 * Deliver pending signals as long as we have the
			 * the opportunity to do so.
			 */
			SIGCHECK(pthread);
			
			return 0;
		}
		
		pthread_lock(&pnext->schedlock);
	}

#ifdef  SCHED_STATS
	stats.switches++;
	stats.switch_cycles += STAT_STAMPDIFF(before);
#endif
#ifdef HIGHRES_THREADTIMES
	{
		long long sstamp;

		pthread_hirestime_get(&sstamp);
		pthread->cpucycles += (sstamp - pthread->rstamp);
		pthread_hirestime_get(&pnext->rstamp);
	}
#endif
#ifdef	THREAD_STATS
	/* XXX somewhat redundent with HIRES_THREADTIMES */
	{
		stat_time_t t;

		/* XXX idle thread is never actually queued */
		if (pthread->runq.next != 0 || pthread == IDLETHREAD) {
			pthread->stats.qstamp = STAT_STAMPGET();
			pthread->stats.preempts++;
		} else {
			pthread->stats.blocks++;
		}
		t = STAT_STAMPDIFF(pthread->stats.rstamp);
		pthread->stats.rtime += t;
		if (t < pthread->stats.rmin)
			pthread->stats.rmin = t;
		if (t > pthread->stats.rmax)
			pthread->stats.rmax = t;

		t = STAT_STAMPDIFF(pnext->stats.qstamp);
		pnext->stats.qtime += t;
		if (t < pnext->stats.qmin)
			pnext->stats.qmin = t;
		if (t > pnext->stats.qmax)
			pnext->stats.qmax = t;
		pnext->stats.scheds++;
		pnext->stats.rstamp = STAT_STAMPGET();
	}
#endif
#ifdef  PTHREAD_SCHED_REALTIME
	if (SCHED_POLICY_REALTIME(pnext->policy))
		ENTER_REALTIME_MODE();
	else
		ENTER_STANDARD_MODE();
#endif
	/*
	 * Switch to next thread. The schedlock for the current thread
	 * is released in the context switch code, while the lock for
	 * next thread is carried across the context switch and is released
	 * once the switch is committed.
	 */
	thread_switch(pnext, &pthread->schedlock, CURPTHREAD());

	/* Must set the current thread pointer! */
	SETCURPTHREAD(pthread);

	/* Call the after context switch hook. */
	if (aftercsw_hook) {
	    (*aftercsw_hook)();
	}

	/*
	 * Thread switch is complete. Unlock the schedlock and proceed.
	 */
	pthread_unlock(&pthread->schedlock);

	/*
	 * Look for exceptions before letting it return to whatever it
	 * was doing before it yielded.
	 */
	if (pthread->sleeprec) {
		/*
		 * Thread is returning to an osenv_sleep. The thread must
		 * be allowed to return, even if it was killed, so the driver
		 * state can be cleaned up. The death will be noticed later.
		 */
		goto done;
	}

	/*
	 * These flag bits are protected by the pthread lock.
	 */
	pthread_lock(&pthread->lock);

	if ((pthread->flags &
	     (THREAD_KILLED|THREAD_EXITING)) == THREAD_KILLED) {
		/*
		 * Thread was canceled. Time to actually reap it, but only
		 * if its not already in the process of exiting.
		 *
		 * XXX: The problem is if the process lock is still
		 * held.  Since the rest of the oskit is not set up to
		 * handle cancelation of I/O operations, let threads
		 * continue to run until the process lock is released.
		 * At that point the death will be noticed.
		 */
		if (! osenv_process_locked()) {
			pthread_exit_locked((void *) PTHREAD_CANCELED);

			/*
			 * Never returns
			 */
		}
		else {
			pthread_unlock(&pthread->lock);
			goto done;
		}
	}
	pthread_unlock(&pthread->lock);

	/*
	 * Look for signals. Deliver them in the context of the thread.
	 */
	SIGCHECK(pthread);

	/* Need to really *restore* the flag since we are in a new context */
	PREEMPT_ENABLE = preemptable;
	return 1;
	

   done:
	/* Need to really *restore* the flag since we are in a new context */
	PREEMPT_ENABLE = preemptable;
	return 1;
}

/*
 * Here starts the external interface functions.
 */

/*
 * Generic switch code. Find a new thread to run and switch to it.
 */
int
pthread_sched_reschedule(resched_flags_t reason, pthread_lock_t *plock)
{
	pthread_thread_t	*pthread = CURPTHREAD();
	int			enabled, rc;

	enabled = save_disable_interrupts();

	/*
	 * Lock the thread schedlock. This provides atomicity with respect
	 * to the switch, since another CPU will will not be able to switch
	 * into a thread until it is completely switched out someplace else.
	 * Once the schedlock is taken, the provided lock can be released,
	 * since the thread can now be woken up, although it cannot be
	 * switched into for the reason just given.
	 */
	assert(plock != &(pthread->lock));

	pthread_lock(&(pthread->schedlock));
	if (plock)
		pthread_unlock(plock);

	rc = pthread_sched_dispatch(reason, 0);

	restore_interrupt_enable(enabled);
	return rc;
}

/*
 * Directed switch. The current thread is blocked with the waitstate.
 */
void
pthread_sched_handoff(int waitstate, pthread_thread_t *pnext)
{
	pthread_thread_t	*pthread = CURPTHREAD();
	int			enabled;

	enabled = save_disable_interrupts();

	if (waitstate) {
		pthread_lock(&pthread->waitlock);
		pthread->waitflags |= waitstate;
		pthread_lock(&pthread->schedlock);
		pthread_lock(&pnext->schedlock);
		pthread_unlock(&pthread->waitlock);
		pthread_sched_dispatch(RESCHED_BLOCK, pnext);
	}
	else {
		pthread_lock(&pthread->schedlock);
		pthread_lock(&pnext->schedlock);
		pthread_sched_dispatch(RESCHED_YIELD, pnext);
	}
	
	restore_interrupt_enable(enabled);
}

/*
 * Initialize schedule parameters for a thread. Called when thread is first
 * created. No locking needed cause no side effects.
 */
void
pthread_sched_init_schedstate(pthread_thread_t *pthread, 
			      int policy, const struct sched_param *param)
{
	int	i;

	pthread->scheduler = 0;
	
	for (i = 0; i < nschedulers; i++) {
		if (scheduler_array[i].schedules &&
		    scheduler_array[i].schedules(policy)) {
			pthread->scheduler = &scheduler_array[i];
			break;
		}
	}

	if (!pthread->scheduler)
		panic(__FUNCTION__ ": Invalid schedule policy!");

	pthread->policy = policy;
	pthread->scheduler->init_schedstate(pthread, param);
}

/*
 * Place a thread that was blocked, back on the runq. The pthread lock
 * is *not* taken. This routine is called from interrupt level to
 * reschedule threads waiting in timed conditions or sleep.
 *
 * Return a boolean value indicating whether the current thread is still
 * the thread that should be running.
 */
int 
pthread_sched_setrunnable(pthread_thread_t *pthread)
{
	int		enabled, resched;
#ifdef  SCHED_STATS
	stat_stamp_t	before;
#endif
	enabled = save_disable_interrupts();
#ifdef  SCHED_STATS
	before = STAT_STAMPGET();
#endif
	pthread_lock(&pthread_sched_lock);

	assert(pthread->scheduler);
	resched = pthread->scheduler->setrunnable(pthread);
	
	pthread_unlock(&pthread_sched_lock);
#ifdef  SCHED_STATS
	stats.wakeups++;
	stats.wakeup_cycles += STAT_STAMPDIFF(before);
#endif
#ifdef	THREAD_STATS
	pthread->stats.qstamp = STAT_STAMPGET();
#endif
	restore_interrupt_enable(enabled);

	return resched;
}

/*
 * Change the state of a thread. In this case, its the priority and
 * policy that are possibly being changed. Return an indicator to the
 * caller, if the change requires a reschedule.
 *
 * Currently, the priority inheritance code is not SMP safe. Also, it is
 * specific to this scheduler module.
 *
 * Does it make sense to allow a thread to change between POSIX and
 * REALTIME? Probably not, but we support it for now. 
 */
int
pthread_sched_change_state(pthread_thread_t *pthread,
			   int policy, const struct sched_param *param)
{
	int		resched = 0;
	
	assert_interrupts_enabled();
	disable_interrupts();

	pthread_lock(&pthread->lock);
	assert(pthread->scheduler);

	if (policy != -1 &&
	    pthread->policy != policy) {
		int	i;

		pthread->scheduler->disassociate(pthread);
		pthread->scheduler = 0;
		
		for (i = 0; i < nschedulers; i++) {
			if (scheduler_array[i].schedules &&
			    scheduler_array[i].schedules(policy))
				pthread->scheduler = &scheduler_array[i];
			break;
		}

		if (!pthread->scheduler)
			panic(__FUNCTION__ ": Invalid schedule policy!");

		pthread->policy = policy;

		pthread->scheduler->init_schedstate(pthread, param);
		resched = 1;
	}
	else
		resched = pthread->scheduler->change_state(pthread, param);

	pthread_unlock(&pthread->lock);
	enable_interrupts();

	return resched;
}

/*
 * Install the context switch hook.  Only one hook can be set.
 * Returns previous hook function.
 */
oskit_pthread_csw_hook_t
oskit_pthread_aftercsw_hook_set(oskit_pthread_csw_hook_t hook)
{
    oskit_pthread_csw_hook_t ohook = aftercsw_hook;
    aftercsw_hook = hook;
    return ohook;
}

#ifdef SIMPLE_PRI_INHERIT
/*
 * The target thread is holding a resource (well, a mutex), and the
 * the current thread wants it, and may or may not be a higher priority.
 * If the current thread *is* higher priority, transfer its priority
 * to the target. When the target unlocks the mutex, its priority will
 * be restored.
 *
 * You cannot transfer *to* a realtime thread.
 */
void
pthread_sched_priority_transfer_and_wait(pthread_thread_t *target,
					 pthread_lock_t *plock)
{
	pthread_thread_t	*pthread = CURPTHREAD();

	assert_interrupts_enabled();
	disable_interrupts();

	pthread_lock(&target->lock);
	if (pthread->priority > target->priority) {
#if 0
		printf("SIMPLE_PRI_TRANSFER: "
		       "0x%x(%p) 0x%x(%p) P:%d --> P:%d\n",
		       (int) pthread, pthread->tid,
		       (int) target, target->tid, 
		       pthread->priority, target->priority);
#endif
		assert(target->scheduler);
		target->scheduler->priority_bump(target, pthread->priority);
	}
	pthread_unlock(&target->lock);
	enable_interrupts();

	pthread_sched_reschedule(RESCHED_BLOCK, plock);
}

/*
 * Restore thread priority.
 */
void
pthread_sched_priority_transfer_undo(pthread_thread_t *pthread)
{
	int	enabled = save_disable_interrupts();

	pthread_lock(&pthread->lock);
	if (pthread->priority != pthread->base_priority) {
#if 0
		printf("SIMPLE_PRI_TRANSFER: "
		       "0x%x(%p) P:%d --> P:%d\n",
		       (int) pthread, pthread->tid,
		       pthread->priority, pthread->base_priority);
#endif
		assert(pthread->scheduler);
		pthread->scheduler->priority_bump(pthread,
						  pthread->base_priority);
	}
	pthread_unlock(&pthread->lock);
	restore_interrupt_enable(enabled);
}
#endif /* SIMPLE_PRI_INHERIT */

#ifdef	SCHED_STATS
void
dump_scheduler_stats()
{
	printf("Wakeups:	%d\n", stats.wakeups);
	printf("Wakeup Cycles   %lu\n", stats.wakeup_cycles);
	printf("Switches:	%d\n", stats.switches);
	printf("Switch Cycles   %lu\n", stats.switch_cycles);
	printf("Switch to same:	        %d\n", stats.switchessame);
	printf("Switch to same Cycles   %lu\n", stats.switchsame_cycles);
}
#endif

#ifdef	THREAD_STATS
void
save_thread_stats(pthread_thread_t *pthread)
{
	php->tid = pthread->tid;
	php->priority = pthread->priority;
	php->stats = pthread->stats;
	if (++php == &pthread_stat_history[PTHREAD_HISTORY_SIZE])
		php = pthread_stat_history;
}

void
dump_thread_stats()
{
	struct pthread_history *p;
	pthread_thread_t *pt;
	int i;
#ifdef LATENCY_THREAD
	extern pthread_thread_t *hiprio;
	extern unsigned long hiprio_missed, hiprio_delayed;
	pthread_thread_t *hpt = hiprio;
#endif		

#ifdef LATENCY_THREAD
	hiprio = 0;
	printf("Hi-priority thread:\n");
	printf("T(%d): scheds %d (%lu/%lu/%lu), delayed %lu, missed %lu\n",
	       hpt->priority, hpt->stats.scheds,
	       hpt->stats.qtime, hpt->stats.qmin, hpt->stats.qmax,
	       hiprio_delayed, hiprio_missed);
#endif
	printf("Idle thread:\n");
	pt = IDLETHREAD;
	printf("T(%d): scheds %d (%lu/%lu/%lu), "
	       "rescheds %d, preempts %d, blocks %d\n",
	       pt->priority, pt->stats.scheds,
	       pt->stats.qtime, pt->stats.qmin, pt->stats.qmax,
	       pt->stats.rescheds, pt->stats.preempts,
	       pt->stats.blocks);
	printf("Current threads:\n");
	for (i = 0; i < THREADS_MAX_THREAD; i++) {
		if ((pt = threads_tidtothread[i]) == 0 ||
		    pt == IDLETHREAD)
			continue;
#ifdef LATENCY_THREAD
		if (pt == hpt)
			continue;
#endif
		printf("T%d(%d): scheds %d (%lu/%lu/%lu), "
		       "rescheds %d, preempts %d, blocks %d\n",
		       (int)pt->tid, pt->priority, pt->stats.scheds,
		       pt->stats.qtime, pt->stats.qmin, pt->stats.qmax,
		       pt->stats.rescheds, pt->stats.preempts,
		       pt->stats.blocks);
	}
	printf("Past threads:\n");
	p = php;
	do {
		if (p->tid != 0)
			printf("T%d(%d): scheds %d (%lu/%lu/%lu), "
			       "rescheds %d, preempts %d, blocks %d\n",
			       (int)p->tid, p->priority, p->stats.scheds,
			       p->stats.qtime, p->stats.qmin, p->stats.qmax,
			       p->stats.rescheds, p->stats.preempts,
			       p->stats.blocks);
		if (++p == &pthread_stat_history[PTHREAD_HISTORY_SIZE])
			p = pthread_stat_history;
	} while (p != php);
}
#endif

#endif /* DEFAULT_SCHEDULER */
