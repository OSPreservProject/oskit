/*
 * Copyright (c) 2000 University of Utah and the Flux Group.
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

#ifdef PTHREAD_SCHED_EDF
#include <threads/pthread_internal.h>
#include "sched_realtime.h"
#include "pthread_signal.h"

/*
 * Simple (degenerate) implementation of Earliest Deadline First (EDF).
 * Non-preemptive. This could be improved greatly.
 *
 * Blocking a realtime task (say, by locking a mutex or doing blocking I/O)
 * is probably a bad thing.
 *
 * I take advantage of the fact that the pthread scheduler does a reschedule
 * check every clock tick. Because of that, I do not have to worry about
 * setting up a preemptor to switch out a non-realtime task when its time
 * for a realtime task to run; the scheduler will check to see if a realtime
 * task is ready to run, and switch out the non-realtime task. This is what
 * makes it degenerate.
 *
 * A more sophisticated approach would be to implement one shot clocks
 * that that would cause a preemption at the proper time. But, the
 * oskit does not have support for that yet. The next version of this
 * code will be better.
 */

/*
 * Non weak symbol to reference so that the library gets dragged in.
 */
int __drag_in_edf_scheduler__;

/*
 * The run queue is ordered by absolute time.
 */
static queue_head_t		edf_runq;
static int			edf_runq_count;

/* pthread_scheduler.c */
extern pthread_lock_t	pthread_sched_lock;

/*
 * These are the internal routines.
 */

/*
 * Determine if a pthread is on the runq. Use a separate field 
 * since using the flags would require locking the thread. Use the
 * queue chain pointer instead, setting it to zero when a thread is
 * removed from the queue.
 */
static inline int
edf_runq_onrunq(pthread_thread_t *pthread)
{
	return (int) pthread->runq.next;
}

/*
 * Add and remove threads from the runq. The runq lock should be locked,
 * and interrupts disabled.
 */

/*
 * Insert a thread into the EDF runq. The queue is ordered by its deadline.
 * As the name implies, the task with the earliest deadline is the first
 * to get run.
 */
static inline void
edf_runq_insert(pthread_thread_t *pthread)
{
	pthread_thread_t	*ptmp;
	
	if (queue_empty(&edf_runq)) {
		queue_enter(&edf_runq, pthread, pthread_thread_t *, runq);
		goto done;
	}
	
	queue_iterate(&edf_runq, ptmp, pthread_thread_t *, runq) {
		if (timespeccmp(&pthread->deadline, &ptmp->deadline, <=)) {
			queue_enter_before(&edf_runq, ptmp,
					   pthread, pthread_thread_t *, runq);
			goto done;
		}
	}
	queue_enter(&edf_runq, pthread, pthread_thread_t *, runq);
   done:
	edf_runq_count++;
#ifdef  THREADS_DEBUG
	/*
	 * This just keeps the deadlock detection code happy.
	 */
	pthread_lock(&threads_sleepers_lock);
	threads_sleepers++;
	pthread_unlock(&threads_sleepers_lock);
#endif
}

/*
 * Remove from runq.
 */
static inline void
edf_runq_remove(pthread_thread_t *pthread)
{
	queue_remove(&edf_runq, pthread, pthread_thread_t *, runq);
	pthread->runq.next = (queue_entry_t) 0;	

	edf_runq_count--;
#ifdef  THREADS_DEBUG
	/*
	 * This just keeps the deadlock detection code happy.
	 */
	pthread_lock(&threads_sleepers_lock);
	threads_sleepers--;
	pthread_unlock(&threads_sleepers_lock);
#endif
}

/*
 * These are the "exported" routines that are required by pthread_scheduler.c
 */

void
edf_sched_init(void)
{
	queue_init(&edf_runq);
	realtime_clock_init();
}

/*
 * Return true of this scheduler schedules the given policy
 */
int
edf_sched_schedules(int policy)
{
	if (policy == SCHED_EDF)
		return 1;

	return 0;
}

/*
 * Make a thread runnable. Called from the scheduler module.
 *
 * It should be noted that blocking a realtime thread is a questionable
 * thing to do!
 */
int
edf_sched_setrunnable(pthread_thread_t *pthread)
{
	int	resched = 0;
	
	if (edf_runq_onrunq(pthread))
		panic("edf_sched_setrunnable: Already on runQ: 0x%x(%d)",
		      (int) pthread, pthread->tid);

	edf_runq_insert(pthread);

#ifdef  PTHREAD_SCHED_POSIX
	/*
	 * If there is a non realtime thread running, preempt it so that
	 * the scheduler can pick the EDF task, if thats what it wants
	 * to do.
	 */
	if (SCHED_POLICY_POSIX(CURPTHREAD()->policy))
		resched = PREEMPT_NEEDED = 1;
#endif

	return resched;
}

/*
 * Terminate our association with a thread, as would happen if the
 * scheduler policy is changed to another scheduler type like realtime.
 *
 * Note, it probably makes no sense to change the policy from one type to
 * another, but you never know ...
 */
void
edf_sched_disassociate(pthread_thread_t *pthread)
{
	pthread_lock(&pthread_sched_lock);
	if (edf_runq_onrunq(pthread))
		edf_runq_remove(pthread);
	pthread_unlock(&pthread_sched_lock);
}

int
edf_sched_check_schedstate(const struct sched_param *param)
{
	/* XXX check something */

	return 0;
}

/*
 * Initialize scheduling parameters for a newly created thread. Note,
 * we don't need any locking since there are no side effects.
 */
void
edf_sched_init_schedstate(pthread_thread_t *pthread,
			       const struct sched_param *param)
{
	/*
	 * The deadline can be different than the start, but I ignore
	 * that distinction for now, and use the start to schedule.
	 */
	pthread->deadline = param->start;
	pthread->period   = param->period;
	pthread->start    = param->start;

	/*
	 * Set the thread priority. We do this so that we can make a
	 * comparison between realtime and nonrealtime threads. I'm
	 * not sure there are any good models for this. This works okay
	 * for keeping queues in proper priority order (say, a mutex
	 * waiters queue).
	 *
	 * We use priorities greater than PRIORITY_MAX so that there is
	 * no interference with POSIX threads, and so that a realtime
	 * thread always appears to have higher priority than any POSIX
	 * thread. This forces rescheduling when appropriate.
	 *
	 * We do not maintain relative priorites (between realtime threads).
	 */
	pthread->priority      = PRIORITY_MAX + 1;
	pthread->base_priority = pthread->priority;
}

/*
 * Change thread scheduler state. pthread is locked. interrupts are disabled.
 */
int
edf_sched_change_state(pthread_thread_t *pthread,
			    const struct sched_param *param)
{
	int	resched = 0;
	
	if (pthread->policy != SCHED_EDF)
		panic("edf_sched_change_state: Bad policy specified");

	/*
	 * The deadline can be different than the start, but I ignore
	 * that distinction for now, and use the start to schedule.
	 */
	pthread->deadline = param->start;
	pthread->period   = param->period;
	pthread->start    = param->start;

	/*
	 * Set the thread priority. We do this so that we can make a
	 * comparison between realtime and nonrealtime threads. I'm
	 * not sure there are any good models for this. This works okay
	 * for keeping queues in proper priority order (say, a mutex
	 * waiters queue).
	 *
	 * We use priorities greater than PRIORITY_MAX so that there is
	 * no interference with POSIX threads, and so that a realtime
	 * thread always appears to have higher priority than any POSIX
	 * thread. This forces rescheduling when appropriate.
	 *
	 * We do not maintain relative priorites (between realtime threads).
	 */
	pthread->priority      = PRIORITY_MAX + 10;
	pthread->base_priority = pthread->priority;
	
	/*
	 * Insert onto runq queue. Changing the parameters of a running
	 * realtime thread may seem like a questionable thing to do ...
	 *
	 * If its not on the runq, then do nothing cause it will get
	 * inserted properly when (re)scheduled.
	 */
	pthread_lock(&pthread_sched_lock);
	if (edf_runq_onrunq(pthread)) {
		edf_runq_remove(pthread);
		edf_runq_insert(pthread);
	}

#ifdef  PTHREAD_SCHED_POSIX
	/*
	 * If there is a non realtime thread running, preempt it so that
	 * the scheduler can pick the EDF task, if thats what it wants
	 * to do.
	 */
	if (SCHED_POLICY_POSIX(CURPTHREAD()->policy))
		resched = PREEMPT_NEEDED = 1;
#endif
	pthread_unlock(&pthread_sched_lock);

	return resched;
}

/*
 * Dispatch a thread back to the runq. The global scheduler lock is locked.
 */
int
edf_sched_dispatch(resched_flags_t reason, pthread_thread_t *pthread)
{
	int	rerun = 0;
	
	switch (reason) {
	case RESCHED_USERYIELD:
		/*
		 * Thread yields. Reschedule at the next deadline.
		 */
		timespecadd(&pthread->deadline, &pthread->period);
		edf_runq_insert(pthread);
		break;
		
	case RESCHED_YIELD:
		/*
		 * We don't allow involuntary based preemption.
		 */
		rerun = 1;
		break;
		
	case RESCHED_PREEMPT:
		/*
		 * We don't allow time based preemption.
		 */
		rerun = 1;
		break;

	case RESCHED_INTERNAL:
		/*
		 * This will be the idle thread. It gets invoked explicitly
		 * by the scheduler, so its deadline does not matter. 
		 */
		break;
		
	default:
		/*
		 * All other rescheduling modes are blocks, and thus ignored.
		 */
		if (pthread == IDLETHREAD)
			panic("posix_sched_dispatch: Idlethread!\n");
		break;
	}
	return rerun;
}

/*
 * Return the highest priority thread ready to run. Scheduler lock is locked.
 */
pthread_thread_t *
edf_sched_thread_next(void)
{
	pthread_thread_t	*pnext;
	extern oskit_timespec_t	threads_realtime;
	
	if (queue_empty(&edf_runq))
		return 0;

	pnext = (pthread_thread_t *) queue_first(&edf_runq);

	/*
	 * The queue is ordered. Just look at the first element of the
	 * queue and see if its deadline is now.
	 */
	if (timespeccmp(&threads_realtime, &pnext->deadline, <))
		return 0;

	/*
	 * Debugging. Remove later.
	 */
	if (timespeccmp(&threads_realtime, &pnext->deadline, >)) {
		oskit_timespec_t	now = threads_realtime;
		
		timespecsub(&now, &pnext->deadline);
		if ((now.tv_nsec / 1000000) > 20) {
			printf("edf_sched_thread_next: "
			       "Thread %p(%d) missed "
			       "its deadline by %d:%d ms\n",
			       pnext, (int) pnext->tid,
			       now.tv_sec, now.tv_nsec / 1000000);
		}
	}
	
	/*
	 * Time has come. Remove and return.
	 */
	edf_runq_remove(pnext);

	return pnext;
}

int
edf_sched_priority_bump(pthread_thread_t *pthread, int newprio)
{
	panic(__FUNCTION__ ": Not supported!");
}
#endif
