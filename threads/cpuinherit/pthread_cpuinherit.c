/*
 * Copyright (c) 1996, 1998, 1999, 2000 University of Utah and the Flux Group.
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

#ifdef	CPU_INHERIT
/*
 * Full Blown CPU Inheritance scheduler.
 */

#ifdef  SMP
Sorry, this code does not work in SMP mode. No locking at all!
#endif

#include <threads/pthread_internal.h>
#include "pthread_signal.h"
#include <stdlib.h>

/*
 * There is a single root scheduler.
 */
pthread_thread_t        *pthread_root_scheduler = NULL_THREADPTR;
int			 cpuinherit_debug       = 0;


#ifdef	MEASURE
#include <oskit/machine/proc_reg.h>

struct pthread_stats {
	int		wakeups;
	int		wakeup_cycles;
	int		wakeup_calls;
	int		switches;
	int		switch_cycles;
	int		donations;
	int		donate_cycles;
	int		handoff_attempts;
	int		handoff_failures;
};
static struct pthread_stats stats;
void	dump_scheduler_stats();

#undef CPUDEBUG
#define CPUDEBUG(cond, args...)
#endif

static inline int
ondonorq(pthread_thread_t *pthread)
{
	return (int) pthread->donors_chain.next;
}

static inline void
donorq_enter(queue_head_t *q, pthread_thread_t *pthread)
{
	queue_enter(q, pthread, pthread_thread_t *, donors_chain);
}

static inline void
donorq_remove(queue_head_t *q, pthread_thread_t *pthread)
{
	queue_remove(q, pthread, pthread_thread_t *, donors_chain);
	pthread->donors_chain.next = 0;
}

/*
 * For debugging.
 */
char *msg_sched_typenames[] = {
	"BADTYPE",
	"UNBLOCK",
	"SETSTATE",
	"EXITED",
	"NEWTHREAD",
	"MAXTYPE",
};

char *msg_sched_rcodes[] = {
	"BADCODE",
	"NOTREADY",
	"BLOCKED",
	"YIELDED",
	"PREEMPTED",
	"TIMEDOUT",
	"BADCODE",
};

char *msg_sched_flagnames[] = {
	"NOTSETYET",
	"DONATING",
	"RUNNING",
	"READY",
	"WAITING",
};

#define STATESTR(p)	(msg_sched_flagnames[(p)->schedflags])

/*
 * There are two sets of functions. The first are routines exported to
 * the core scheduling system. The second are routines exported to the
 * schedulers. The first opertate on thread pointers directly. The second
 * indirect through thread TIDs.
 */

/*
 * Initialize the scheduler subsystem.
 */
void
pthread_init_scheduler(void)
{
	char	*bp;

	if ((bp = getenv("CPUDEBUG")) != NULL)
		cpuinherit_debug = strtol(bp, NULL, 0);

	if (cpuinherit_debug)
	    printf("CPUINHERIT_DEBUG = 0x%x\n", cpuinherit_debug);
#ifdef MEASURE
	atexit(dump_scheduler_stats);
#endif
}

/*
 * A thread is being resumed. Send a message to the threads scheduler.
 */
int 
pthread_sched_setrunnable(pthread_thread_t *pthread)
{
	pthread_sched_wakeup(pthread, 0);

	/*
	 * Return indicator for yield.
	 */
	return CURPTHREAD()->preempt;
}

/*
 * Change the state of a thread. Send a message to the threads scheduler.
 * When pthread_yield is called, another message is going to be generated,
 * buts thats okay. This first message will change the thread state and
 * the second message will be the yield message for whatever thread is
 * actually running (might be different than the thread being changed).
 */
int
pthread_sched_change_state(pthread_thread_t *pthread, int newprio, int policy)
{
	schedmsg_t		msg;

	msg.type    = MSG_SCHED_SETSTATE;
	msg.tid     = pthread->tid;
	msg.opaque  = newprio;
	msg.opaque2 = policy;

	/*
	 * Send it an IPC. This thread will block if the target scheduler
	 * is not waiting for a message.
	 */
	pthread_sched_message_send(pthread->scheduler, &msg);	
	
	/*
	 * Return indicator for yield.
	 */
	return CURPTHREAD()->preempt;
}

/*
 * Helper function for creating schedulers. Force a switch into a specific
 * thread. Nothing special about it, except that target thread must remain
 * locked across the switch. 
 */
void
pthread_sched_switchto(pthread_thread_t *pthread)
{
	pthread_thread_t	*cur = CURPTHREAD();
	int			enabled, preemptable;

	save_preemption_enable(preemptable);
	enabled = save_disable_interrupts();
	
	/* Don't worry about the lock */
	thread_switch(pthread, &cur->schedlock, CURPTHREAD());

	/* Must set the current thread pointer! */
	SETCURPTHREAD(cur);

	restore_interrupt_enable(enabled);
	/* Need to really *restore* the flag since we are in a new context */
	PREEMPT_ENABLE = preemptable;
}

/*
 * Switch out of an application thread, back into its scheduler. Return
 * value to indicate whether a switch actually happened.
 */
int
pthread_sched_dispatch(resched_flags_t reason)
{
	pthread_thread_t	*pthread = CURPTHREAD();
	pthread_thread_t	*inheritor, *donator, *nextup;
	int			enabled, preemptable;
#ifdef  MEASURE
	unsigned long		before, after;
#endif	
	save_preemption_enable(preemptable);
	enabled = save_disable_interrupts();

#ifdef  MEASURE
	before = get_tsc();
#endif
	/*
	 * See what donator should receive the message and get restarted.
	 * Go back up the inheritance chain. Stop at the root scheduler
	 * (which will have set WAKEUP_ALWAYS) or when some intermediate
	 * thread says it wants to be woken up.
	 */
	donator   = pthread->inherits_from;
	nextup    = pthread->nextup;
	inheritor = pthread;
	assert(inheritor != donator);

	/*
	 * Better be a donator unless its the root scheduler or idlethread.
	 */
	assert(donator || ((pthread == pthread_root_scheduler) ||
			   (pthread == IDLETHREAD)));

	/*
	 * The current thread state is set accordingly.
	 */
	if (reason == RESCHED_BLOCK)
		pthread->schedflags = SCHED_WAITING;
	else
		pthread->schedflags = SCHED_READY;

	/*
	 * Go back through the inheritance links, stopping at the first
	 * thread to indicate that it wants to be woken up. That thread
	 * is left locked across the thread switch. It is responsible for
	 * unlocking itself when it gets to the other side.
	 */
	while (donator) {
		switch (reason) {
		case RESCHED_USERYIELD:
			donator->donate_rc = SCHEDULE_YIELDED;
			break;
		case RESCHED_BLOCK:
			donator->donate_rc = SCHEDULE_BLOCKED;
			break;
		case RESCHED_PREEMPT:	/* time based preemption */
			donator->donate_rc = SCHEDULE_TIMEDOUT;
			break;
		default:
			donator->donate_rc = SCHEDULE_PREEMPTED;
			break;
		}

		/*
		 * With each step back through the active chain, set
		 * the thread state accordingly. Blocks are different
		 * then yields in that another thread should be able
		 * to donate to a thread that yielded. However, blocked
		 * chains should cause a wakeup for any thread along
		 * the chain.
		 */
		if (reason == RESCHED_BLOCK)
			donator->schedflags = SCHED_WAITING;
		else
			donator->schedflags = SCHED_READY;

		if (nextup) {
			if (nextup == donator &&
			    (nextup = donator->nextup) == NULL_THREADPTR)
				break;
		}
		else {
			if (donator->wakeup_cond == WAKEUP_ALWAYS ||
			    donator->wakeup_cond == WAKEUP_ON_BLOCK)
				break;
		}

		inheritor = donator;
		donator   = donator->inherits_from;
	}
	
	if (donator == NULL_THREADPTR) {
		/*
		 * Nothing to run, not even the root scheduler has anything
		 * to do. Control reaches here when the root scheduler
		 * calls pthread_sched_wait(), or when the idle thread
		 * attempts a reschedule in hopes of finding something to
		 * do.
		 */
		if ((donator = IDLETHREAD) == pthread) {
			restore_interrupt_enable(enabled);
			return 0;
		}

		/*
		 * Rather kludgy method to keep the idle loop an orphan.
		 * The root scheduler does not know about the idle loop,
		 * so don't want to terminate its message wait when the
		 * idle loop does its yield to look for a new thread.
		 */
		donator->inherits_from = NULL_THREADPTR;
		donator->nextup        = NULL_THREADPTR;
		
		goto dispatch;
	}

#if 0
	/*
	 * Encode a return status for the donator so that it knows what
	 * to do with the thread after it switches out.
	 *
	 * special flag indicates that the donation chain was modified
	 * by a wakeup along the chain. The return code of the thread
	 * being switched into was already set in the wakeup code, and
	 * giving that thread a return code based on what the current
	 * thread is doing makes no sense.
	 *
	 * The only reason to distingiush between all these reschedule
	 * reasons is so that we can implement a POSIX style scheduler.
	 */
	switch (reason) {
	case RESCHED_USERYIELD:
		donator->donate_rc = SCHEDULE_YIELDED;
		break;
	case RESCHED_BLOCK:
		donator->donate_rc = SCHEDULE_BLOCKED;
		break;
	case RESCHED_PREEMPT:	/* time based preemption */
		donator->donate_rc = SCHEDULE_TIMEDOUT;
		break;
	default:
		donator->donate_rc = SCHEDULE_PREEMPTED;
		break;
	}
#endif

dispatch:
	/*
	 * Clear directed yield flag.
	 */
	PREEMPT_NEEDED = pthread->preempt = 0;
	assert(donator);

#ifdef  MEASURE
	after = get_tsc();
	if (after > before) {
		stats.switches++;
		stats.switch_cycles += (after - before);
	}
#endif
	/* Don't worry about the lock */
	thread_switch(donator, &pthread->schedlock, CURPTHREAD());

	/* Must set the current thread pointer! */
	SETCURPTHREAD(pthread);

	assert(pthread->inherits_from ||
	       ((pthread == pthread_root_scheduler) ||
		(pthread == IDLETHREAD)));
	
	/*
	 * Thread has switched back in. 
	 *
	 * Time to look for exceptions before letting it return to
	 * whatever it was doing before it yielded.
	 */
	pthread->schedflags = SCHED_RUNNING;
	
	if (pthread->sleeprec) {
		/*
		 * Thread is returning to an osenv_sleep. The thread must
		 * be allowed to return, even if it was killed, so the driver
		 * state can be cleaned up. The death will be noticed later.
		 */
		goto done;
	}
	else if ((pthread->flags &
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
		else
			goto done;
	}
	
	/*
	 * Look for signals. Deliver them in the context of the thread.
	 */
	SIGCHECK(pthread);

   done:
	restore_interrupt_enable(enabled);
	/* Need to really *restore* the flag since we are in a new context */
	PREEMPT_ENABLE = preemptable;
	return 1;
}

/*
 * Generic switch code. Find a new thread to run and switch to it.
 */
int
pthread_sched_reschedule(resched_flags_t reason, pthread_lock_t *plock)
{
	int			enabled, rc;

	enabled = save_disable_interrupts();

	/*
	 * No thread locking, but need to release the provided lock
	 * in case it was built with locks enabled.
	 */
	if (plock)
		pthread_unlock(plock);
	
	rc = pthread_sched_dispatch(reason);

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

	/*
	 * Not sure how to handle this yet. Just yield and expect that
	 * pnext's scheduler will wakeup and run it.
	 */
	if (! waitstate) {
		pthread_sched_wakeup(pnext, 0);
		pthread_sched_dispatch(RESCHED_YIELD);
		restore_interrupt_enable(enabled);
		return;
	}

	pthread_sched_wakeup(pnext, 0);
	pthread->waitflags |= waitstate;
	
#ifdef  MEASURE
	stats.handoff_attempts++;
#endif
	if (!pthread_sched_thread_donate(pnext, WAKEUP_NEVER, 0)) {
		pthread_sched_dispatch(RESCHED_BLOCK);
#ifdef  MEASURE
		stats.handoff_failures++;
#endif
	}
	restore_interrupt_enable(enabled);
}

/*
 * Wakeup call.
 */
void
pthread_sched_wakeup(pthread_thread_t *pthread, int level)
{
	int			enabled;
	pthread_thread_t	*pscheduler = pthread->scheduler;
	pthread_thread_t	*inheritor, *ptmp;
#ifdef  MEASURE
	unsigned long		before, after;
#endif
	enabled = save_disable_interrupts();

#ifdef  MEASURE
	before = get_tsc();
#endif
	CPUDEBUG(CORE0,
		 "pthread_sched_wakeup: "
		 "C:0x%x(%d)/%s T:0x%x(%d)/%s S:0x%x(%d)/%s\n",
		 (int) CURPTHREAD(), CURPTHREAD()->tid, STATESTR(CURPTHREAD()),
		 (int) pthread, pthread->tid, STATESTR(pthread),
		 (int) pscheduler,
		 (pscheduler ? pscheduler->tid : 0),
		 (pscheduler ? STATESTR(pscheduler) : ""));


	/*
	 * Thread is currently ready to run. The wrinkle is if the thread
	 * was part of a donation chain that yielded, or a donation chain
	 * that was interrupted by a wakeup. There will be a donating_to
	 * link, which must be severed so that when the chain is run again,
	 * the thread in question will run, rather then following the
	 * chain.
	 */
	if (pthread->schedflags == SCHED_READY) {
		if (pthread->donating_to) {
			CPUDEBUG(CORE0,
				 "PSW0: 0x%x(%d) 0x%x 0x%x\n",
				 (int) pthread, pthread->tid,
				 (int) pthread->donating_to,
				 (int) pthread->scheduler);
			
			pthread->donate_rc   = SCHEDULE_PREEMPTED;
			pthread->donating_to = NULL_THREADPTR;
			pthread->wakeup_cond = WAKEUP_ALWAYS;
		}
		goto done;
	}

	/*
	 * Thread is donating, which means its on the active chain.
	 */
	if (pthread->schedflags == SCHED_DONATING) {
		pthread->schedflags = SCHED_READY;
		
		/*
		 * Follow the chain down, setting each one along the way
		 * to the ready state instead of donating. 
		 */
		inheritor = pthread->donating_to;
		
		while (inheritor->donating_to) {
			inheritor->schedflags = SCHED_READY;

			ptmp      = inheritor;
			inheritor = inheritor->donating_to;
		}
		
		/*
		 * Go to the end of the chain. Severe the chain at the
		 * thread being woken up. Point the last thread in the
		 * chain (usually the thread actually running) back at the
		 * this thread. This means that when the running thread
		 * switches out, it will follow its link back to this
		 * thread. When it runs, it will return from its donation,
		 * so set the return code so that it reruns whatever thread
		 * it was donating to.
		 */
		pthread->donating_to     = NULL_THREADPTR;
		pthread->donate_rc       = SCHEDULE_PREEMPTED;
		pthread->wakeup_cond     = WAKEUP_ALWAYS;
		inheritor->nextup        = pthread;

		/*
		 * The wrinkle is if the last thread in the chain is not
		 * the currently running thread. That means the chain
		 * below this point was already woken up (it was severed).
		 * Need to entend the backwards chain so that when the
		 * currently running thread switches out, it follows the
		 * links back to this thread (along multiple hops). The
		 * intent is to run the highest level scheduler that wants
		 * to run.
		 */
		if (inheritor != CURPTHREAD())
			inheritor->wakeup_cond = WAKEUP_NEVER;

		PREEMPT_NEEDED = CURPTHREAD()->preempt = 1;

		CPUDEBUG(CORE3,
			 "PSW1: 0x%x(%d)\n", (int) inheritor, inheritor->tid);
			
		goto done;
	}

	/*
	 * The thread is not on the active chain, and its not in the READY
	 * state (already woken up). Need to follow the chain back until we
	 * get to a thread *is* DONATING.
	 */
	CPUDEBUG(CORE1, "PSW2:\n");

	/*
	 * Want to wake up either the thread donating to it, or its scheduler.
	 * Of course, they might be the same.
	 */
	if (pthread->inherits_from &&
	    pthread->inherits_from != pscheduler) {
		/*
		 * A non-scheduler thread was donating to another thread,
		 * and the inheritor blocked. Now the inheritor is being
		 * woken up. Need to wakeup the donator in the hopes that
		 * it can provide some more CPU.
		 */
		/* cpuprintf("PSW3: 0x%x 0x%x 0x%x\n",
		       (int) pthread, (int) pscheduler,
		       (int) pthread->inherits_from); */
		
		pthread->schedflags = SCHED_READY;
		inheritor = pthread->inherits_from;
		
		pthread_sched_wakeup(inheritor, level + 1);
	}
	else if (pscheduler) {
		pthread->schedflags = SCHED_READY;
		
		/*
		 * If the thread waking up is different than the thread
		 * the scheduler is currently donating to, this implies
		 * two (or more) threads, and thus a conflict. Set the
		 * wakeup_cond back to WAKEUP_ALWAYS so that the scheduler
		 * will be woken up.
		 */
		if (pscheduler->donating_to &&
		    pscheduler->donating_to != pthread) {
			pscheduler->wakeup_cond = WAKEUP_ALWAYS;

			CPUDEBUG(CORE1,
				 "PSW4: 0x%x\n",
				 (int) pscheduler->donating_to);
		}

		/*
		 * If the scheduler is WAKEUP_ON_SWITCH, then the
		 * above case was not true, and the scheduler still has
		 * only one thread to run. The scheduler *is* woken up, 
		 * but since it will donate to that thread anyway, do
		 * not send it a message.
		 */
		if (pscheduler->wakeup_cond == WAKEUP_ALWAYS ||
		    pscheduler->wakeup_cond == WAKEUP_ON_BLOCK) {
			schedmsg_t	msg;
			
			CPUDEBUG(CORE2,
				 "PSW5: 0x%x(%d) 0x%x(%d)\n",
				 (int) pscheduler, pscheduler->tid,
				 (int) pthread, pthread->tid);

			msg.type    = MSG_SCHED_UNBLOCK;
			msg.tid     = pthread->tid;
			msg.opaque  = 0;
			pthread_sched_special_send(pscheduler, &msg);
		}
		else {
			pthread_sched_wakeup(pscheduler, level + 1);
		}
	}
	else {
		/*
		 * Hit the root scheduler, which was not running.
		 * That means the current thread (the idle thread)
		 * should switch back into the root scheduler.
		 * Set the back link on the idle thread to force this
		 * to happen.
		 */
		assert(pthread == pthread_root_scheduler);

		if (pthread->schedflags != SCHED_RUNNING) {
			pthread->schedflags         = SCHED_READY;
			CURPTHREAD()->nextup        = pthread;
			CURPTHREAD()->inherits_from = pthread;
		}
	}
   done:
#ifdef  MEASURE
	after = get_tsc();
	if (level) {
		stats.wakeup_calls++;
	}
	else if (after > before) {
		stats.wakeups++;
		stats.wakeup_cycles += (after - before);
	}
#endif
	restore_interrupt_enable(enabled);
}

/*
 * Internal thread donation call.
 *
 * The current thread lock must be locked.
 */
int
pthread_sched_thread_donate(pthread_thread_t *pchild,
			    sched_wakecond_t wakeup_cond, oskit_s32_t timeout)
{
	pthread_thread_t	*pnext, *pthread = CURPTHREAD();
	int			enabled, preemptable;
#ifdef  MEASURE
	unsigned long		before, after;
#endif
	save_preemption_enable(preemptable);
	enabled = save_disable_interrupts();

#ifdef  MEASURE
	before = get_tsc();
#endif
	CPUDEBUG(CORE3,
		 "PSD0: 0x%x(%d) 0x%x(%d) %s\n",
		 (int) pthread, pthread->tid,
		 (int) pchild, pchild->tid, STATESTR(pchild));

	/*
	 * If the thread is not ready to accept this new donation, just
	 * return status that the donation was not accepted.
	 */
	if (pchild->schedflags != SCHED_READY) {
		CPUDEBUG(CORE3,
			 "PSD1: 0x%x(%d) 0x%x(%d) %s\n",
			 (int) pthread, pthread->tid,
			 (int) pchild, pchild->tid, STATESTR(pchild));

		restore_interrupt_enable(enabled);
		return 0;
	}

	/*
	 * Is another thread already donating to this thread? 
	 */
	if (pchild->inherits_from) {
		CPUDEBUG(CORE3,
			 "PSD2: 0x%x(%d) 0x%x(%d) 0x%x(%d)\n",
			 (int) pthread, pthread->tid,
			 (int) pchild, pchild->tid,
			 (int) pchild->inherits_from,
			 pchild->inherits_from->tid);
	}

	/*
	 * We are commited to the donation.
	 */

	/*
	 * The target thread is going to inherit from the current thread.
	 */
	pchild->inherits_from = pthread;
	pchild->nextup        = NULL_THREADPTR;
	
	/*
	 * BUT, if the thread being switched into is part of an old active
	 * chain, need to decend the chain setting each thread to DONATED.
	 * Set the back pointer too since it might no longer be valid if
	 * another thread donated to the chain at some point.
	 * 
	 * STOP when we hit a thread with WAKEUP_ALWAYS set, or at the end.
	 * This is the thread that actually gets run. Lock that thread.
	 */
	pnext = pchild;
	if (pnext->donating_to) {
		while (pnext->donating_to) {
			pthread_thread_t	*ptmp;
			
			if (pnext->schedflags != SCHED_READY)
				panic("pthread_sched_donate SCHED_READY");

			if (pnext->wakeup_cond == WAKEUP_ALWAYS)
				break;
		
			pnext->schedflags = SCHED_DONATING;

			/*
			 * Goto next in the chain. Lock the next one,
			 * unlock the current one, set the back pointer.
			 */
			ptmp = pnext->donating_to;
			ptmp->inherits_from = pnext;
			ptmp->nextup        = NULL_THREADPTR;
			
			CPUDEBUG(CORE3,
				 "PSD3: "
				 "Followed chain from 0x%x(%d) to 0x%x(%d)\n",
				 (int) pnext, pnext->tid,
				 (int) pnext->donating_to,
				 pnext->donating_to->tid);

			pnext = ptmp;
		}
	}

	/*
	 * Set up the links.
	 */
	pthread->schedflags   = SCHED_DONATING;
	pthread->donating_to  = pchild;
	pthread->wakeup_cond  = wakeup_cond;
	pthread->timeout      = timeout;
	pnext->schedflags     = SCHED_RUNNING;

#ifdef  MEASURE
	after = get_tsc();
	if (after > before) {
		stats.donations++;
		stats.donate_cycles += (after - before);
	}
#endif
	/* Don't worry about the lock */
	thread_switch(pnext, &pthread->schedlock, CURPTHREAD());

	/* Must set the current thread pointer! */
	SETCURPTHREAD(pthread);

	/*
	 * Back from donation. 
	 *
	 * Undo the donating link. Leave the back link alone since another
	 * thread could have donated to the child if the chain was preempted.
	 */
	pthread->timeout      = 0;
	pthread->donating_to  = NULL_THREADPTR;
	pthread->schedflags   = SCHED_RUNNING;

	if (pchild->inherits_from == pthread)
		pchild->inherits_from = NULL_THREADPTR;

	restore_interrupt_enable(enabled);
	/* Need to really *restore* the flag since we are in a new context */
	PREEMPT_ENABLE = preemptable;
	return 1;
}

/*
 * Priority inheritance entrypoint. Donate CPU time to a thread being
 * waited for.
 */
void
pthread_sched_thread_wait(pthread_thread_t *waiting_on,
			  queue_head_t *queue, pthread_lock_t *plock)
{
	pthread_thread_t	*pthread = CURPTHREAD();
	int			enabled;

	enabled = save_disable_interrupts();
	
	if (queue)
		queue_enter(queue, pthread, pthread_thread_t *, chain);

	/*
	 * No thread locking, but need to release the provided lock
	 * in case it was built with locks enabled.
	 */
	if (plock)
		pthread_unlock(plock);
	
/*	cpuprintf("pthread_sched_thread_wait: h:0x%x(%d) p:0x%x(%d)\n",
		  (int) waiting_on, waiting_on->tid,
		  (int) pthread, pthread->tid); */

	/*
	 * Try for the donation. If it fails, fall back to a simple
	 * reschedule. Note that the pthread lock is still locked.
	 */
	if (!pthread_sched_thread_donate(waiting_on, WAKEUP_NEVER, 0))
		pthread_sched_reschedule(RESCHED_BLOCK, &pthread->lock);

	restore_interrupt_enable(enabled);
}

/*
 * Clock tick handler. Called from pthread_interrupt (interrupts disabled).
 */
void
pthread_sched_clocktick(void)
{
	pthread_thread_t	*pthread;
	int			needsoftint = 0;

	if (! PREEMPT_ENABLE)
		return;

	pthread = CURPTHREAD();

	/*
	 * Look for preemption along the active chain. These are threads
	 * that donated with a timeout value. Decrement the timeout and
	 * schedule any threads that expire. 
	 */
	while (pthread) {
		if (pthread->timeout) {
			pthread->timeout -= PTHREAD_TICK;
			
			if (pthread->timeout <= 0) {
			CPUDEBUG(CORE3,
				 "PRE: 0x%x(%d) 0x%x 0x%x\n",
				 (int) pthread, pthread->tid,
				 (int) pthread->donating_to,
				 (int) pthread->scheduler);

				
				pthread->timeout     = 0;
				pthread->wakeup_cond = WAKEUP_ALWAYS;
				pthread_sched_wakeup(pthread, 0);
				pthread->donate_rc   = SCHEDULE_TIMEDOUT;

				needsoftint = 1;
			}
		}
		pthread = pthread->inherits_from;
	}

	if (needsoftint && CURPTHREAD() != pthread_root_scheduler)
		softint_request(SOFTINT_TIMEOUT);
}

/*
 * These are the routines that are exported to schedulers.
 */

/*
 * A scheduler tells us its a scheduler.
 */
void
pthread_sched_become_scheduler(void)
{
	pthread_thread_t	*pthread = CURPTHREAD();

	pthread->msgqueue = schedmsg_queue_allocate(128);
}

/*
 * Change the scheduling parameters for a thread. Currently a hack that
 * sends a message to the threads scheduler containing a single opaque
 * value that hopefully makes sense to the scheduler. This will need to
 * beef up at some point.
 */
int
pthread_sched_setstate(pthread_t tid, int opaque)
{
	pthread_thread_t	*pthread;
	schedmsg_t		msg;

	if ((pthread = tidtothread(tid)) == NULL_THREADPTR)
		return EINVAL;

	msg.type    = MSG_SCHED_SETSTATE;
	msg.tid     = pthread->tid;
	msg.opaque  = opaque;

	/*
	 * Send it an IPC. This thread will block if the target scheduler
	 * is not waiting for a message.
	 */
	pthread_sched_message_send(pthread->scheduler, &msg);	

	return 0;
}

/*
 * This is the basic scheduling call invoked to donate CPU time to a
 * thread.  We may receive a message when some other thread wakes up
 * from a timeout, so we disable interrupts.
 */
int
pthread_sched_donate_wait_recv(pthread_t tid,
	sched_wakecond_t wakeup_cond, schedmsg_t *msg, oskit_s32_t timeout)
{
	pthread_thread_t	*pthread = CURPTHREAD();
	pthread_thread_t	*ptarget;
	int			rc;
	
	if ((ptarget = tidtothread(tid)) == NULL_THREADPTR)
		panic("pthread_sched_donate: Bad TID: %d\n", tid);

	assert_preemption_enabled();
	assert_interrupts_enabled();
	disable_interrupts();

	/*
	 * Setup the scheduler to be woken up from its donation if
	 * a message is sent to it. The thread is made to look like it 
	 * is in a message wait.
	 */
	if (pthread_sched_recv_wait(pthread, msg)) {
		/*
		 * Oops, there is a message pending so give that
		 * back to the caller instead of doing a donation.
		 */
		enable_interrupts();
		return SCHEDULE_MSGRECV;
	}

	/*
	 * Donate. 
	 */
	rc = pthread_sched_thread_donate(ptarget, wakeup_cond, timeout);

	if (!rc) {
		/*
		 * If the donation fails, undo the recv_wait. The
		 * thread is still locked. No message could have been
		 * delivered, so no need to check.
		 */
		pthread_sched_recv_unwait(pthread, msg);
		enable_interrupts();
		return SCHEDULE_NOTREADY;
	}

	/*
	 * Donation succeeded. There *might* be a message, so must clear
	 * the wait and check to see if a message is pending.
	 */
	rc = pthread->donate_rc;

	if (pthread_sched_recv_unwait(pthread, msg))
		rc |= SCHEDULE_MSGRECV;

	enable_interrupts();
	return rc;
}

/*
 * Debugging!
 */
#include <stdarg.h>
#include <unistd.h>

int
cpuprintf(const char *fmt, ...)
{
	va_list	args;
	int err, enabled;
	char buf[256];

	enabled = save_disable_interrupts();
	va_start(args, fmt);
	err = vsnprintf(buf, 256, fmt, args);
	va_end(args);
	write(1, buf, strlen(buf));
	restore_interrupt_enable(enabled);

	return err;
}

#ifdef	MEASURE
void
dump_scheduler_stats()
{
	printf("Wakeups:	%d\n", stats.wakeups);
	printf("Wakeup Cycles   %u\n", stats.wakeup_cycles);
	printf("Wakeup Calls:	%d\n", stats.wakeup_calls);
	printf("Switches:	%d\n", stats.switches);
	printf("Switch Cycles   %u\n", stats.switch_cycles);
	printf("Donations:	%d\n", stats.donations);
	printf("Donate Cycles   %u\n", stats.donate_cycles);
	printf("Handoff Tries:	%d\n", stats.handoff_attempts);
	printf("Handoff Fails   %d\n", stats.handoff_failures);
}
#endif
#endif /* CPU_INHERIT */

#ifndef KNIT
void
foobar()
{
}
#endif
