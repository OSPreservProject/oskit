/*
 * Copyright (c) 2000, 2001 University of Utah and the Flux Group.
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

#ifdef PTHREAD_SCHED_STRIDE
#include <threads/pthread_internal.h>
#include "pthread_signal.h"

/*
 * Stride Scheduler.
 * http://www.research.digital.com/SRC/personal/caw/papers/stride-mit-tm528.ps
 */

/*
 * Non weak symbol to reference so that the library gets dragged in.
 */
int __drag_in_stride_scheduler__;

/*
 * The run queue is ordered by the pass value.
 */
static queue_head_t		stride_runq;
static int			stride_runq_count;
static int			global_tickets;	
static int			global_stride;	
static long long		global_pass;
static long long		lastupdate;
static int			stride_debug = 0;

/*
 * Stride constants.
 */
#define STRIDE1		(1 << 20)
#define SCALE		1000000
#define QUANTUM		(PTHREAD_TICK * SCALE)

/*
 * Shorthand
 */
#define TICKETS(p)	(p->tickets)
#define STRIDE(p)	(p->stride)
#define PASS(p)		(p->pass)
#define REMAIN(p)	(p->remain)
#define START(p)	(p->start)

/* pthread_scheduler.c */
extern pthread_lock_t	pthread_sched_lock;

#ifdef STRIDE_DISABLE
static int		stride_disabled;
static long long	global_pass_save;
#define FIXEDTICKETS	100
#endif

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
stride_runq_onrunq(pthread_thread_t *pthread)
{
	return (int) pthread->runq.next;
}

/*
 * Add and remove threads from the runq. The runq lock should be locked,
 * and interrupts disabled.
 */

/*
 * Insert into the runq. Simple ordered queue. Might need to be better!
 */
static void
stride_runq_insert(pthread_thread_t *pthread)
{
	pthread_thread_t	*ptmp;
	
	if (queue_empty(&stride_runq)) {
		queue_enter(&stride_runq, pthread, pthread_thread_t *, runq);
		goto done;
	}
	
	queue_iterate(&stride_runq, ptmp, pthread_thread_t *, runq) {
		if (PASS(pthread) < PASS(ptmp)) {
			queue_enter_before(&stride_runq, ptmp,
					   pthread, pthread_thread_t *, runq);
			goto done;
		}
	}
	queue_enter(&stride_runq, pthread, pthread_thread_t *, runq);
   done:
	stride_runq_count++;
}

/*
 * Dequeue the highest priority thread, which is the first thread since
 * the list ordered by PASS.
 */
static pthread_thread_t *
stride_runq_dequeue(void)
{
	pthread_thread_t	*pnext;

	queue_remove_first(&stride_runq, pnext, pthread_thread_t *, runq);
	pnext->runq.next = (queue_entry_t) 0;
	stride_runq_count--;
	START(pnext) = oskit_pthread_realtime();

	return pnext;
}

/*
 * Remove an arbitrary thread from the runq.
 */
static inline void
stride_runq_remove(pthread_thread_t *pthread)
{
	queue_remove(&stride_runq, pthread, pthread_thread_t *, runq);
	pthread->runq.next = (queue_entry_t) 0;	
	stride_runq_count--;
}

/*
 * Update global pass based on elapsed real time.
 */
static void
global_pass_update(void)
{
	long long	elapsed;

#ifdef  STRIDE_DISABLE
	/* charge everyone for a full tick */
        if (stride_disabled)
		elapsed = PTHREAD_TICK;
	else
#endif
	elapsed     = oskit_pthread_realtime() - lastupdate;
	lastupdate += elapsed;

	global_pass += ((long long) global_stride *
			(elapsed * SCALE)) / QUANTUM;
		
	assert(global_pass >= 0);
}

/*
 * Update thread pass when a thread does not consume its entire quantum,
 * but is put back on the runq. Say, cause it was preempted.
 */
static void
thread_pass_update(pthread_thread_t *pthread)
{
	long long	elapsed;
	int		stride = STRIDE(pthread);

	elapsed  = oskit_pthread_realtime();
	elapsed -= START(pthread);

#ifdef  STRIDE_DISABLE
        if (stride_disabled)
		stride = STRIDE1 / FIXEDTICKETS;
#endif
	PASS(pthread) += (stride * (elapsed * SCALE)) / QUANTUM;

	assert(PASS(pthread) >= 0);
}

/*
 * Update global tickets and stride to reflect change in runq.
 */
static void 
global_tickets_update(int delta)
{
#ifdef  STRIDE_DISABLE
        if (stride_disabled) {
		if (delta >= 0)
			delta = FIXEDTICKETS;
		else
			delta = -FIXEDTICKETS;
	}
#endif
	global_tickets += delta;

	assert(global_tickets >= 0);

	/*
	 * XXX: Is this the correct thing to do when the last thread
	 * leaves? The paper says nothing about it.
	 */
	if (global_tickets)
		global_stride = STRIDE1 / global_tickets;
	else
		global_stride = STRIDE1;
}

/*
 * Initialize the tickets for a new thread to some reasonable value.
 */
static void
client_init(pthread_thread_t *pthread, int tickets)
{
	TICKETS(pthread) = tickets;
	STRIDE(pthread)  = STRIDE1 / tickets;
	REMAIN(pthread)  = STRIDE(pthread);
}

/*
 * Join the party.
 */
static void
client_join(pthread_thread_t *pthread)
{
	global_pass_update();
	
	PASS(pthread) = global_pass + REMAIN(pthread);
	global_tickets_update(TICKETS(pthread));
	
	stride_runq_insert(pthread);
}

/*
 * Leave the party
 */
static void
client_leave(pthread_thread_t *pthread)
{
	global_pass_update();
	
#ifdef  STRIDE_DISABLE
	/* no credit for unused time */
        if (stride_disabled)
		REMAIN(pthread) = 0;
	else
#endif
	REMAIN(pthread) = PASS(pthread) - global_pass;
	global_tickets_update(-TICKETS(pthread));
}

/*
 * Change priority
 */
static void
client_modify(pthread_thread_t *pthread, int current, int tickets)
{
	int	remain, stride, queued;

	if ((queued = stride_runq_onrunq(pthread))) {
		stride_runq_remove(pthread);
		client_leave(pthread);
	}
	else if (current)
		global_tickets_update(-TICKETS(pthread));

	stride = STRIDE1 / tickets;
	remain = (REMAIN(pthread) * stride) / STRIDE(pthread);

	TICKETS(pthread) = tickets;
	STRIDE(pthread)  = stride;
	REMAIN(pthread)  = remain;

	if (queued)
		client_join(pthread);
	else if (current)
		global_tickets_update(TICKETS(pthread));
}

/*
 * Debug
 */
static void
stride_runq_debug(void)
{
	pthread_thread_t	*pthread;

	printf(__FUNCTION__ ": GT %d GS %d GP %qd LU %qd\n",
	       global_tickets, global_stride, global_pass, lastupdate);
	
	queue_iterate(&stride_runq, pthread, pthread_thread_t *, runq) {
		printf("%p(%d) T %d S %d P %qd R %d S %qd\n",
		       pthread, (int) pthread->tid,
		       TICKETS(pthread), STRIDE(pthread), PASS(pthread),
		       REMAIN(pthread), START(pthread));
	}
}

/*
 * These are the "exported" routines that are required by pthread_scheduler.c
 */

void
stride_sched_init(void)
{
	queue_init(&stride_runq);
}

/*
 * Return true of this scheduler schedules the given policy
 */
int
stride_sched_schedules(int policy)
{
	if (policy == SCHED_STRIDE)
		return 1;

	return 0;
}

/*
 * Make a thread runnable. Called from the scheduler module. Global scheduler
 * lock is locked already!
 */
int
stride_sched_setrunnable(pthread_thread_t *pthread)
{
	int	resched = 0;
	
	if (stride_runq_onrunq(pthread))
		panic(__FUNCTION__ ": Already on runQ: 0x%x(%d)",
		      (int) pthread, pthread->tid);

	client_join(pthread);

#ifdef  PTHREAD_SCHED_POSIX
	/*
	 * If there is a POSIX thread running, preempt it so that the
	 * scheduler can pick the next task, if thats what it wants to
	 * do.
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
stride_sched_disassociate(pthread_thread_t *pthread)
{
	pthread_lock(&pthread_sched_lock);
	if (stride_runq_onrunq(pthread)) {
		stride_runq_remove(pthread);
		client_leave(pthread);
	}
	pthread_unlock(&pthread_sched_lock);
}

/*
 * Validate proposed scheduler state.  Returns 0 if ok, an error otherwise.
 */
int
stride_sched_check_schedstate(const struct sched_param *param)
{
	return (param->tickets > 0 && param->tickets <= STRIDE1) ? 0 : EINVAL;
}

/*
 * Initialize scheduling parameters for a newly created thread. Note,
 * we don't need any locking since there are no side effects.
 */
void
stride_sched_init_schedstate(pthread_thread_t *pthread,
			     const struct sched_param *param)
{
	static int	initdone;
	
	if (pthread->policy != SCHED_STRIDE)
		panic(__FUNCTION__ ": Bad policy specified");
	if (param->tickets <= 0 || param->tickets > STRIDE1)
		panic(__FUNCTION__ ": Bad ticket value specified");

	assert_preemption_enabled();
	disable_preemption();
	pthread_lock(&pthread_sched_lock);

	client_init(pthread, param->tickets);

	/*
	 * XXX!!!
	 */
	if (! initdone) {
		/*
		 * Must be main thread, which is already running!
		 */
		global_tickets_update(TICKETS(pthread));
		global_pass_update();
		PASS(pthread) = global_pass;
		initdone = 1;
	}

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
	 */
	pthread->priority      = PRIORITY_MAX + 5;
	pthread->base_priority = pthread->priority;
	
#ifdef  PTHREAD_SCHED_POSIX
	/*
	 * If there is a POSIX thread running, preempt it so that the
	 * scheduler can pick the next task, if thats what it wants to
	 * do.
	 */
	if (SCHED_POLICY_POSIX(CURPTHREAD()->policy))
		PREEMPT_NEEDED = 1;
#endif

	pthread_unlock(&pthread_sched_lock);
	enable_preemption();
}

/*
 * Change thread scheduler state. pthread is locked. interrupts are disabled.
 */
int
stride_sched_change_state(pthread_thread_t *pthread,
			  const struct sched_param *param)
{
	int	resched = 0;
	
	if (pthread->policy != SCHED_STRIDE)
		panic(__FUNCTION__ ": Bad policy specified");
	if (param->tickets <= 0 || param->tickets > STRIDE1)
		panic(__FUNCTION__ ": Bad ticket value specified");

	pthread_lock(&pthread_sched_lock);

	client_modify(pthread, CURPTHREAD() == pthread, param->tickets);

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
	 */
	pthread->priority      = PRIORITY_MAX + 5;
	pthread->base_priority = pthread->priority;
	
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
stride_sched_dispatch(resched_flags_t reason, pthread_thread_t *pthread)
{
	int	rerun = 0;
	
	switch (reason) {
	case RESCHED_USERYIELD:
	case RESCHED_YIELD:
	case RESCHED_PREEMPT:
		/*
		 * Thread yields. Reschedule.
		 */
		assert(! stride_runq_onrunq(pthread));
		thread_pass_update(pthread);
		stride_runq_insert(pthread);
		break;

	case RESCHED_INTERNAL:
		/*
		 * This will be the idle thread. It gets invoked explicitly
		 * by the scheduler, so its tickets/pass/stride/whatever
		 * do not matter. 
		 */
		break;
		
	default:
		/*
		 * All other rescheduling modes are blocks, and thus ignored.
		 */
		if (pthread == IDLETHREAD)
			panic("posix_sched_dispatch: Idlethread!\n");
		thread_pass_update(pthread);
		client_leave(pthread);
		break;
	}
	return rerun;
}

/*
 * Return the highest priority thread ready to run. Scheduler lock is locked.
 */
pthread_thread_t *
stride_sched_thread_next(void)
{
	pthread_thread_t	*pnext;

	if (stride_debug)
		stride_runq_debug();
	
	if (queue_empty(&stride_runq))
		return 0;

	pnext = stride_runq_dequeue();

	return pnext;
}

int
stride_sched_priority_bump(pthread_thread_t *pthread, int newprio)
{
	panic(__FUNCTION__ ": Not supported!");
}

#ifdef STRIDE_DISABLE
void
stride_scheduler_enable(void)
{
	int		 enabled;
	pthread_thread_t *ptmp;
	queue_head_t	 runq_copy;

	if (!stride_disabled)
		return;

	enabled = save_disable_interrupts();

	stride_runq_debug();

        queue_init(&runq_copy);
	ptmp              = CURPTHREAD();
	stride_disabled   = 0;
	global_tickets    = 0;
	global_tickets_update(TICKETS(ptmp));
	PASS(ptmp)        = global_pass_save +
		            (STRIDE(ptmp) * (PTHREAD_TICK * SCALE)) / QUANTUM;
        global_pass       = PASS(ptmp);
	stride_runq_count = 0;

	if (queue_empty(&stride_runq)) {
		restore_interrupt_enable(enabled);
		stride_runq_debug();
		return;
	}
		
	while (! queue_empty(&stride_runq)) {
		queue_remove_first(&stride_runq,
				   ptmp, pthread_thread_t *, runq);

		queue_enter(&runq_copy, ptmp, pthread_thread_t *, runq);
	}

	while (! queue_empty(&runq_copy)) {
		queue_remove_first(&runq_copy,
				   ptmp, pthread_thread_t *, runq);

		PASS(ptmp) = global_pass_save +
   			     (STRIDE(ptmp) * (PTHREAD_TICK * SCALE)) / QUANTUM;
		
		global_tickets_update(TICKETS(ptmp));
		stride_runq_insert(ptmp);
	}
	ptmp = (pthread_thread_t *) queue_first(&stride_runq);
	global_pass = PASS(ptmp);
	
	stride_runq_debug();

	restore_interrupt_enable(enabled);
}

void
stride_scheduler_disable(void)
{
	int		 enabled;
	pthread_thread_t *ptmp;
	
	enabled = save_disable_interrupts();

	stride_runq_debug();

	ptmp             = CURPTHREAD();
	global_pass_save = global_pass;
	stride_disabled  = 1;
	global_tickets   = 0;
	global_tickets_update(FIXEDTICKETS);
	PASS(ptmp)       = global_pass +
		(STRIDE(ptmp) * (PTHREAD_TICK * SCALE)) / QUANTUM;

	queue_iterate(&stride_runq, ptmp, pthread_thread_t *, runq) {
		PASS(ptmp) = global_pass +
			(STRIDE(ptmp) * (PTHREAD_TICK * SCALE)) / QUANTUM;
		global_tickets_update(FIXEDTICKETS);
	}
	
	stride_runq_debug();

	restore_interrupt_enable(enabled);
}
#endif
#endif
