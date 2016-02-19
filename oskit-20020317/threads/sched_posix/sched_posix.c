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

/*
 * Default POSIX SCHED_FIFO and SCHED_RR policies ...
 */

#include <threads/pthread_internal.h>
#include "pthread_signal.h"
#define  COMPILING_SCHEDULER
#include "sched_posix.h"

/* pthread_scheduler.c */
extern pthread_lock_t	pthread_sched_lock;

/*
 * The RunQ is a multilevel queue of doubly linked lists. Use a bitmask
 * to indicate whichrunq is non-empty, with the least significant bit
 * being the highest priority (cause off ffs).
 */
#define MAXPRI		(PRIORITY_MAX + 1)

/*
 * These are internal to the scheduler.
 */
static queue_head_t	threads_runq[MAXPRI] = { {0} };
static int		threads_runq_count   = 0;
static oskit_u32_t	threads_whichrunq;

extern int		ffs();

/*
 * These are the internal routines.
 */

/*
 * Are there any threads on the runq?
 */
static inline int
posix_runq_empty(void)
{
	return (threads_whichrunq == 0);
}

/*
 * Get the highest priority scheduled thread.
 */
static inline int
posix_runq_maxprio(void)
{
	int	prio;
	
	if (posix_runq_empty())
		return -1;
	else {
		prio = ffs(threads_whichrunq);
		
		return PRIORITY_MAX - (prio - 1);
	}
}

/*
 * Determine if a pthread is on the runq. Use a separate field 
 * since using the flags would require locking the thread. Use the
 * queue chain pointer instead, setting it to zero when a thread is
 * removed from the queue.
 */
static inline int
posix_runq_onrunq(pthread_thread_t *pthread)
{
	return (int) pthread->runq.next;
}

/*
 * Add and remove threads from the runq. The runq lock should be locked,
 * and interrupts disabled.
 */

/*
 * Insert at the tail of the runq.
 */
static inline void
posix_runq_insert_tail(pthread_thread_t *pthread)
{
	int		prio  = PRIORITY_MAX - pthread->priority;
	queue_head_t	*phdr = &threads_runq[prio];

	queue_enter(phdr, pthread, pthread_thread_t *, runq);

	threads_whichrunq |= (1 << prio);
	threads_runq_count++;
}

/*
 * Insert at the head of the runq.
 */
static inline void
posix_runq_insert_head(pthread_thread_t *pthread)
{
	int		prio  = PRIORITY_MAX - pthread->priority;
	queue_head_t	*phdr = &threads_runq[prio];

	queue_enter_first(phdr, pthread, pthread_thread_t *, runq);

	threads_whichrunq |= (1 << prio);
	threads_runq_count++;
}

/*
 * Dequeue highest priority pthread.
 */
static inline pthread_thread_t *
posix_runq_dequeue(void)
{
	int			prio  = ffs(threads_whichrunq) - 1;
	queue_head_t		*phdr = &threads_runq[prio];
	pthread_thread_t	*pnext;

	queue_remove_first(phdr, pnext, pthread_thread_t *, runq);
	pnext->runq.next = (queue_entry_t) 0;	

	threads_runq_count--;
	if (queue_empty(phdr))
		threads_whichrunq &= ~(1 << prio);

	return pnext;
}

/*
 * Remove an arbitrary thread from the runq.
 */
static inline void
posix_runq_remove(pthread_thread_t *pthread)
{
	int		prio  = PRIORITY_MAX - pthread->priority;
	queue_head_t	*phdr = &threads_runq[prio];

	queue_remove(phdr, pthread, pthread_thread_t *, runq);
	pthread->runq.next = (queue_entry_t) 0;	

	threads_runq_count--;
	if (queue_empty(phdr))
		threads_whichrunq &= ~(1 << prio);
}

/*
 * These are the "exported" routines that are required by pthread_scheduler.c
 */

void
posix_sched_init(void)
{
	int	i;
		
	for (i = 0; i < MAXPRI; i++)
		queue_init(&threads_runq[i]);
}

/*
 * Return true of this scheduler schedules the given policy
 */
int
posix_sched_schedules(int policy)
{
	if (policy == SCHED_RR || policy == SCHED_FIFO)
		return 1;

	return 0;
}

/*
 * Make a thread runnable. Called from the scheduler module.
 */
int
posix_sched_setrunnable(pthread_thread_t *pthread)
{
	int	resched = 0;
	
	if (posix_runq_onrunq(pthread))
		panic("posix_sched_setrunnable: Already on runQ: 0x%x(%d)",
		      (int) pthread, pthread->tid);

	posix_runq_insert_tail(pthread);

	if ((CURPTHREAD()->priority < posix_runq_maxprio()) &&
	    SCHED_POLICY_POSIX(CURPTHREAD()->policy))
		resched = PREEMPT_NEEDED = 1;

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
posix_sched_disassociate(pthread_thread_t *pthread)
{
	pthread_lock(&pthread_sched_lock);
	if (posix_runq_onrunq(pthread)) {
		/*
		 * On the scheduler queue, so its not running.
		 */
		posix_runq_remove(pthread);
	}
	pthread_unlock(&pthread_sched_lock);
}

int
posix_sched_check_schedstate(const struct sched_param *param)
{
	/* XXX check something here */

	return 0;
}

/*
 * Initialize scheduling parameters for a newly created thread. Note,
 * we don't need any locking since there are no side effects.
 */
void
posix_sched_init_schedstate(pthread_thread_t *pthread,
			    const struct sched_param *param)
{
	assert(SCHED_POLICY_POSIX(pthread->policy));

	pthread->base_priority = param->priority;
	pthread->priority      = param->priority;
	pthread->ticks         = SCHED_RR_INTERVAL;
}

/*
 * Change thread scheduler state. pthread is locked. interrupts are disabled.
 */
int
posix_sched_change_state(pthread_thread_t *pthread,
			 const struct sched_param *param)
{
	int	newprio = param->priority;
	int	resched = 0;

	if (! (pthread->policy & (SCHED_FIFO|SCHED_RR)))
		panic("posix_sched_change_state: Bad policy specified");

	if (pthread->base_priority == newprio)
	    goto done;

	pthread->base_priority = newprio;

#ifdef  PRI_INHERIT
	if (pthread->base_priority < pthread->priority) {
		if (! pthread->inherits_from)
			pthread_priority_decreasing_recompute(pthread);
	}
	else {
		pthread->priority = newprio;
		if (pthread->waiting_for)
			pthread_priority_increasing_recompute(pthread);

		if (pthread->inherits_from &&
		    newprio > pthread->inherits_from->priority) {
			pthread->inherits_from = NULL_THREADPTR;
		}
	}
	pthread_lock(&pthread_sched_lock);
#else
	pthread_lock(&pthread_sched_lock);
	if (posix_runq_onrunq(pthread)) {
		posix_runq_remove(pthread);
		pthread->priority = newprio;
		posix_runq_insert_tail(pthread);
	}
	else {
		/*
		 * If its not on the runq, the thread priority can
		 * just be changed since there are no current
		 * dependencies on it. If however, the current thread
		 * has its priority changed, a reschedule might be
		 * necessary.
		 */
		pthread->priority = newprio;
	}
#endif	
	if ((CURPTHREAD()->priority < posix_runq_maxprio()) &&
	    SCHED_POLICY_POSIX(CURPTHREAD()->policy))
		resched = PREEMPT_NEEDED = 1;

	pthread_unlock(&pthread_sched_lock);

 done:
	return resched;
}

/*
 * Bump thread priority, as for priority transfer. Transfer is temporary.
 */
int
posix_sched_priority_bump(pthread_thread_t *pthread, int newprio)
{
	int	resched = 0;

	if (pthread->priority == newprio)
	    goto done;

	/*
	 * Realtime threads have higher priorities. Cut the new priority
	 * back to whats allowed for the POSIX scheduler.
	 *
	 * XXX Should PRIORITY_MAX actually be 30 instead of 31, to make
	 * room for transfers from a realtime thread.
	 */
	if (newprio > PRIORITY_MAX)
		newprio = PRIORITY_MAX;

	pthread_lock(&pthread_sched_lock);
	if (posix_runq_onrunq(pthread)) {
		/*
		 * On the scheduler queue, so its not running.
		 */
		posix_runq_remove(pthread);
		pthread->priority = newprio;
		posix_runq_insert_tail(pthread);
	}
	else {
		/*
		 * If its not on the runq, the thread priority can
		 * just be changed since there are no current dependencies
		 * on it (running or blocked). If however, the current thread
		 * has its priority changed, a reschedule might be
		 * necessary.
		 */
		pthread->priority = newprio;
	}
	if ((CURPTHREAD()->priority < posix_runq_maxprio()) &&
	    SCHED_POLICY_POSIX(CURPTHREAD()->policy))
		resched = PREEMPT_NEEDED = 1;

	pthread_unlock(&pthread_sched_lock);

 done:
	return resched;
}

/*
 * Dispatch a thread back to the runq.
 */
int
posix_sched_dispatch(resched_flags_t reason, pthread_thread_t *pthread)
{
	switch (reason) {
	case RESCHED_USERYIELD:
		if (pthread == IDLETHREAD)
			panic("posix_sched_dispatch: Idlethread!\n");

		/*
		 * A user directed yield forces the thread to the back
		 * of the queue.
		 */
		posix_runq_insert_tail(pthread);
		break;
		
	case RESCHED_YIELD:
		if (pthread == IDLETHREAD)
			panic("posix_sched_dispatch: Idlethread!\n");

		/*
		 * A involuntary yield forces the thread to the front 
		 * of the queue. It will probably be rerun right away!
		 */
		posix_runq_insert_head(pthread);
		break;
		
	case RESCHED_PREEMPT:
		if (pthread == IDLETHREAD)
			panic("posix_sched_dispatch: Idlethread!\n");

		/*
		 * Time based preemption. If the policy is RR, then the
		 * thread goes to the back of the queue if it has used
		 * all of its ticks. Otherwise it stays at the head, of
		 * course.
		 *
		 * If the policy is FIFO, this has no real effect, other
		 * than to possibly allow a higher priority thread to get
		 * the CPU.
		 */
		if (pthread->policy == SCHED_RR) {
			if (--pthread->ticks == 0) {
				posix_runq_insert_tail(pthread);
				pthread->ticks = SCHED_RR_INTERVAL;
			}
			else
				posix_runq_insert_head(pthread);
		}
		else if (pthread->policy == SCHED_FIFO)
			posix_runq_insert_head(pthread);
		break;

	case RESCHED_INTERNAL:
		/*
		 * This will be the idle thread.
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
	return 0;
}

/*
 * Return the highest priority thread ready to run.
 */
pthread_thread_t *
posix_sched_thread_next(void)
{
	if (posix_runq_empty())
		return 0;

	return posix_runq_dequeue();
}

#ifdef  PRI_INHERIT
#ifdef  SMP
Sorry, this code does not work in SMP mode. No locking at all!
#endif

/*
 * This priority inheritance algorithm is based on the paper "Removing
 * Priority Inversion from an Operating System," by Steven Sommer.
 */
int	threads_priority_debug = 0;

void	pthread_priority_decreasing_recompute(pthread_thread_t *pthread);
void	pthread_priority_increasing_recompute(pthread_thread_t *pthread);

OSKIT_INLINE void
threads_change_priority(pthread_thread_t *pthread, int newprio)
{
	if (posix_runq_onrunq(pthread)) {
		posix_runq_remove(pthread);
		pthread->priority = newprio;
		posix_runq_insert_tail(pthread);
	}
	else
		pthread->priority = newprio;
}

void
pthread_priority_inherit(pthread_thread_t *pwaiting_for)
{
	pthread_thread_t	*pthread = CURPTHREAD();
	int			enabled;

	enabled = save_disable_interrupts();

	/*
	 * Add blocked thread to targets' list of waiters.
	 */
	queue_enter(&pwaiting_for->waiters, pthread,
		    pthread_thread_t *, waiters_chain);

	/*
	 * Set the waiting link so that the chain of waiters
	 * can be followed when doing the priority transfer.
	 */
	pthread->waiting_for = pwaiting_for;

	pthread_priority_increasing_recompute(pthread);

	restore_interrupt_enable(enabled);
}

/*
 * Do a transitive priority transfer. Following the waiting_for links,
 * transfering higher priority to lower priority threads.
 *
 * pthread is locked.
 */
void
pthread_priority_increasing_recompute(pthread_thread_t *pthread)
{
	pthread_thread_t	*pwaiting_for = pthread->waiting_for;

	do {
		if (pthread->priority <= pwaiting_for->priority)
			break;

		if (threads_priority_debug)
			printf("Inherit: Transferring priority: "
			       "From %p(%d/%d) to %p(%d/%d)\n",
			       pthread, (int) pthread->tid, pthread->priority,
			       pwaiting_for, (int) pwaiting_for->tid,
			       pwaiting_for->priority);

		threads_change_priority(pwaiting_for, pthread->priority);
		pwaiting_for->inherits_from = pthread;
		
		pthread = pwaiting_for;
		pwaiting_for = pwaiting_for->waiting_for;
	} while (pwaiting_for);
}	

/*
 * Undo the effects of a priority inheritance after a thread unblocks
 * another thread.
 */
void
pthread_priority_uninherit(pthread_thread_t *punblocked)
{
	pthread_thread_t	*pthread = CURPTHREAD();
	pthread_thread_t	*ptmp1;
	int			enabled;

	enabled = save_disable_interrupts();
	
	/*
	 * Remove the unblocked thread from the set of threads blocked
	 * by the current thread.
	 */
	queue_remove(&pthread->waiters,
		     punblocked, pthread_thread_t *, waiters_chain);

	/*
	 * The rest of the waiters are now waiting on the unblocked thread
	 * since it won the prize. Indicate that, and add the entire set
	 * of threads to the list of threads that punblocked is blocking.
	 */
	while (! queue_empty(&pthread->waiters)) {
		queue_remove_first(&pthread->waiters,
				   ptmp1, pthread_thread_t *, waiters_chain);

		ptmp1->waiting_for = punblocked;
		
		queue_enter(&punblocked->waiters,
			    ptmp1, pthread_thread_t *, waiters_chain);
	}
	punblocked->waiting_for = NULL_THREADPTR;

	/*
	 * Now recompute the priorities.
	 */
	if (pthread->inherits_from)
		pthread_priority_decreasing_recompute(pthread);
		      
	restore_interrupt_enable(enabled);
}

void
pthread_priority_decreasing_recompute(pthread_thread_t *pthread)
{
	int			priority = pthread->priority;
	pthread_thread_t	*pnext = 0, *ptmp;
	int			maxpri = -1;

	/*
	 * Find the highest priority thread from the set of threads
	 * waiting on pthread.
	 */
	queue_iterate(&pthread->waiters, ptmp, pthread_thread_t *, chain) {
		if (ptmp->priority > maxpri) {
			maxpri = ptmp->priority;
			pnext  = ptmp;
		}
	}

	/*
	 * If there is a waiting thread, and its priority is greater
	 * then this threads' priority, then inherit priority
	 * from that thread. Otherwise, reset this threads' priority
	 * back to its base priority since either there is nothing to
	 * inherit from (empty waiters), or its base priority is better
	 * than any of the waiting threads.
	 */
	if (pnext) {
		if (threads_priority_debug)
			printf("Uninherit: Transferring priority: "
			       "From %p(%d/%d) to %p(%d/%d)\n",
			       pnext, (int) pnext->tid, pnext->priority,
			       pthread, (int) pthread->tid, pthread->priority);

		if (pnext->priority > pthread->base_priority) {
			threads_change_priority(pthread, pnext->priority);
			pthread->inherits_from = pnext;
		}
	}
	else {
		if (threads_priority_debug)
			printf("Resetting priority back to base: "
			       "%p(%d) - %d to %d\n",
			       pthread, (int) pthread->tid,
			       pthread->priority, pthread->base_priority);

		threads_change_priority(pthread, pthread->base_priority);
		pthread->inherits_from = 0;
	}

	/*
	 * If this threads' priority was lowered, and another thread is
	 * inheriting from it, must propogate the lowered priority down
	 * the chain. This would not happen when a thread unblocks another
	 * thread. It could happen if an external event caused a blocked
	 * thread to change state, and that thread was donating its priority.
	 */
	if (pthread->priority < priority) {
		if (pthread->waiting_for &&  
		    pthread->waiting_for->inherits_from == pthread) {
			pthread_priority_decreasing_recompute(pthread->
							      waiting_for);
		}
	}
}

/*
 * A thread is killed, but waiting on a thread. Must undo the inheritance.
 *
 * Interrupts should be disabled, and the thread locked.
 */
void
pthread_priority_kill(pthread_thread_t *pthread)
{
	pthread_thread_t	*pwaiting_for = pthread->waiting_for;
	
	queue_remove(&pwaiting_for->waiters, pthread,
		     pthread_thread_t *, waiters_chain);

	pthread->waiting_for = NULL_THREADPTR;
	
	if (pwaiting_for->inherits_from == pthread)
		pthread_priority_decreasing_recompute(pwaiting_for);
}
#endif
