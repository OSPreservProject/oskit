/*
 * Copyright (c) 1996, 1998, 1999 University of Utah and the Flux Group.
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
 * Root scheduler for CPU inheritance scheduling system. Implements (or
 * attempts to) the POSIX style fixed priority scheduler for SCHED_RR and
 * SCHED_FIFO. Certain elements of this are a bit confused since its not
 * always clear why the thread being scheduled stopped running, so adhering
 * to the POSIX rules is difficult (translation: not done).
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <oskit/threads/pthread.h>
#include <oskit/threads/cpuinherit.h>
#include <oskit/queue.h>
#include <malloc.h>
#include <assert.h>
#include "hash.h"

#ifdef	MEASURE
#include <oskit/machine/proc_reg.h>

struct fixedpri_stats {
	int		msgs;
	int		msg_cycles;
};
static struct fixedpri_stats stats;
void		dump_fixedpri_stats();

#undef CPUDEBUG
#define CPUDEBUG(cond, args...)
#endif

/*
 * Basically the same scheduler as the builtin version.
 */
#define MAXPRI			(PRIORITY_MAX + 1)
#define SCHED_RR_INTERVAL	1

/*
 * The scheduler structures are per-scheduler instantiation.
 *
 * The RunQ is a multilevel queue of doubly linked lists. Use a bitmask
 * to indicate whichrunq is non-empty, with the least significant bit
 * being the highest priority (cause of ffs).
 *
 * TIDs are mapped back to the state structure using a key value.
 */
typedef struct fixedpri_sched {
	queue_head_t		runq[MAXPRI];
	int			runq_count;
	oskit_u32_t		runq_mask;
	short			ready;
	short			preemptible;
	pthread_t		maintid;	/* *Is* the root scheduler */
	pthread_t		tid;		/* Back pointer to thread */
	hash_table_t	       *hashtable;	/* Hash TID to pstate */
#ifdef  DEBUG
	struct fixedpri_state  *threads[128];
#endif
} fixedpri_sched_t;

/*
 * Per-thread state information structure.
 */
typedef struct fixedpri_state {
	queue_chain_t	runq;			/* Queueing element */
	pthread_t	tid;			/* TID of thread */
	int		policy;			/* FIFO or RR */
	int		priority;
	int		ticks;
	int		base_priority;
#ifdef  DEBUG
	int		lastcode;
#endif
} fixedpri_state_t;

#define NULL_FPSTATE	((fixedpri_state_t *) 0)

extern int		ffs();
void		       *fixedpri_schedloop(void *arg);
static fixedpri_sched_t *allocate_fixedpri_scheduler(void);
static void		fixedpri_canceled(void *arg);
static int		fixedpri_pedantic = 1;

/*
 * Map TID to pstate structure with a hash table.
 */
static void
fixedpri_setstate(fixedpri_sched_t *psched,
		  pthread_t tid, fixedpri_state_t *pstate)
{
	if (tidhash_add(psched->hashtable, (void *) pstate, tid))
		panic("fixedpri_setstate: "
		      "hashtable add failed: tid:%d pstate:0x%x",
		      tid, (int) pstate);
}

static fixedpri_state_t *
fixedpri_getstate(fixedpri_sched_t *psched, pthread_t tid)
{
	fixedpri_state_t	*pstate;

	if ((pstate = (fixedpri_state_t *)
	     tidhash_lookup(psched->hashtable, tid)) == NULL)
		panic("fixedpri_getstate: "
		      "hashtable lookup failed: tid:%d", tid);

	return pstate;
}

static void
fixedpri_remstate(fixedpri_sched_t *psched, pthread_t tid)
{
	tidhash_rem(psched->hashtable, tid);
}

/*
 * Special hook to bootstrap the root scheduler. Return the entrypoint
 * of the scheduler, and the data structure to pass as the first argument.
 * The caller will create a new thread with those values and run it.
 * The caller passes in the TID of the main thread. Create the state
 * structure for it as well.
 */
void
bootstrap_root_scheduler(pthread_t tid, int preemptible,
			 void *(**function)(void *), void **argument)
{
	fixedpri_state_t	*pstate;
	fixedpri_sched_t	*psched;

	/*
	 * Allocate the scheduler data structure.
	 */
	psched = allocate_fixedpri_scheduler();
	psched->preemptible = preemptible;
	psched->maintid     = tid;

	/*
	 * Allocate the per-thread data structure for the main thread.
	 */
	if ((pstate =
	     (fixedpri_state_t *) calloc(1, sizeof(fixedpri_state_t))) == NULL)
		panic("fixedpri_thread_init: No more memory");

	pstate->tid           = tid;
	pstate->priority      = PRIORITY_NORMAL;
	pstate->base_priority = PRIORITY_NORMAL;
	pstate->policy        = SCHED_RR;
	pstate->ticks         = SCHED_RR_INTERVAL;
	fixedpri_setstate(psched, tid, pstate);
#ifdef  DEBUG
	psched->threads[(int) tid]  = pstate;
#endif
	*argument = psched;
	*function = fixedpri_schedloop;
}

/*
 * Create a fixedpri scheduler. This creates the thread and makes sure
 * it gets run so that it exists and is ready to handle scheduling
 * messages.
 */
int
create_fixedpri_scheduler(pthread_t *tid,
			  const pthread_attr_t *attr, int preemptible)
{
	fixedpri_sched_t		*psched;

	psched = allocate_fixedpri_scheduler();
	psched->preemptible = preemptible;

	pthread_create(tid, attr, fixedpri_schedloop, (void *) psched);

	/*
	 * The scheduler has to run. 
	 */
	while (! psched->ready)
		oskit_pthread_sleep(1);

	/*
	 * Back in this thread. Just return.
	 */
	return 0;
}

/*
 * Allocate and initialize a per-instantiation root scheduler runq.
 */
static fixedpri_sched_t *
allocate_fixedpri_scheduler(void)
{
	fixedpri_sched_t	*psched;
	int		i;
	
	if ((psched = (fixedpri_sched_t *)
	     calloc(1, sizeof(fixedpri_sched_t))) == NULL)
		panic("allocate_fixedpri_scheduler: No more memory");

	if (tidhash_create(&psched->hashtable, 0))
		panic("allocate_fixedpri_scheduler: Hash Table allocation");
		
	for (i = 0; i < MAXPRI; i++)
		queue_init(&(psched->runq[i]));

	return psched;
}

/*
 * Initialize the per-thread scheduler state structure. This is called
 * when a new thread is created. The state structure is then stored
 * in the key table so that TIDs can be mapped back to the state structure.
 */
static fixedpri_state_t *
fixedpri_thread_init(fixedpri_sched_t *psched,
		     pthread_t tid, int priority, int policy)
{
	fixedpri_state_t	*pstate;

	if ((pstate =
	     (fixedpri_state_t *) calloc(1, sizeof(fixedpri_state_t))) == NULL)
		panic("fixedpri_thread_init: No more memory");

	pstate->tid           = tid;
	pstate->priority      = priority;
	pstate->base_priority = priority;
	pstate->policy        = policy;
	pstate->ticks         = SCHED_RR_INTERVAL;
	fixedpri_setstate(psched, tid, pstate);
#ifdef  DEBUG
	psched->threads[(int) tid]  = pstate;
#endif
	return pstate;
}	

/*
 * Are there any threads on the runq?
 */
static inline int
runq_empty(fixedpri_sched_t *psched)
{
	return (psched->runq_mask == 0);
}

/*
 * Get the highest priority scheduled thread.
 */
static inline int
runq_maxprio(fixedpri_sched_t *psched)
{
	int	prio;
	
	if (runq_empty(psched))
		return -1;
	else {
		prio = ffs(psched->runq_mask);
		
		return PRIORITY_MAX - (prio - 1);
	}
}

/*
 * Determine if a thread is on the runq. Use a separate field 
 * since using the flags would require locking the thread. Use the
 * queue chain pointer instead, setting it to zero when a thread is
 * removed from the queue.
 */
static inline int
runq_onrunq(fixedpri_sched_t *psched, fixedpri_state_t *pstate)
{
	return (int) pstate->runq.next;
}

/*
 * Add and remove threads from the runq. The runq lock should be locked,
 * and interrupts disabled.
 */

/*
 * Insert at the tail of the runq.
 */
static inline void
runq_insert_tail(fixedpri_sched_t *psched, fixedpri_state_t *pstate)
{
	int		prio  = PRIORITY_MAX - pstate->priority;
	queue_head_t	*phdr = &(psched->runq[prio]);

	queue_enter(phdr, pstate, fixedpri_state_t *, runq);

	psched->runq_mask  |= (1 << prio);
	psched->runq_count += 1;
}

/*
 * Insert at the head of the runq.
 */
static inline void
runq_insert_head(fixedpri_sched_t *psched, fixedpri_state_t *pstate)
{
	int		prio  = PRIORITY_MAX - pstate->priority;
	queue_head_t	*phdr = &(psched->runq[prio]);

	queue_enter_first(phdr, pstate, fixedpri_state_t *, runq);

	psched->runq_mask  |= (1 << prio);
	psched->runq_count += 1;
}

/*
 * Dequeue highest priority pthread.
 */
static fixedpri_state_t *
runq_dequeue(fixedpri_sched_t *psched)
{
	int			prio  = ffs(psched->runq_mask) - 1;
	queue_head_t		*phdr = &(psched->runq[prio]);
	fixedpri_state_t	*pnext;

	queue_remove_first(phdr, pnext, fixedpri_state_t *, runq);
	pnext->runq.next = (queue_entry_t) 0;	

	psched->runq_count--;
	if (queue_empty(phdr))
		psched->runq_mask &= ~(1 << prio);

	return pnext;
}

/*
 * Debug the runq.
 */
static void
runq_check(fixedpri_sched_t *psched)
{
	int			count, i;
	fixedpri_state_t	*pstate;
	
	count = 0;
	for (i = 0; i < MAXPRI; i++) {
		if (! queue_empty(&(psched->runq[i]))) {
			queue_iterate(&(psched->runq[i]), 
				      pstate, fixedpri_state_t *, runq) {
				count++;
			}
		}
	}
	if (count != psched->runq_count)
		panic("Bad runq(%d): 0x%x\n",
		      pthread_self(), psched);
}

/*
 * Remove an arbitrary thread from the runq.
 */
static inline void
runq_remove(fixedpri_sched_t *psched, fixedpri_state_t *pstate)
{
	int		prio  = PRIORITY_MAX - pstate->priority;
	queue_head_t	*phdr = &(psched->runq[prio]);

	queue_remove(phdr, pstate, fixedpri_state_t *, runq);
	pstate->runq.next = (queue_entry_t) 0;	

	psched->runq_count--;
	if (queue_empty(phdr))
		psched->runq_mask &= ~(1 << prio);
}

/*
 * Debug
 */
static void
runq_debug(fixedpri_sched_t *psched)
{
	int			i;
	fixedpri_state_t	*pstate;
	
	printf("fixedpri(%d): Count:%d  Mask:0x%x\n", (int) psched->tid,
	       psched->runq_count, psched->runq_mask);
	
	for (i = 0; i < MAXPRI; i++) {
		if (! queue_empty(&(psched->runq[i]))) {
			queue_iterate(&(psched->runq[i]), 
				      pstate, fixedpri_state_t *, runq) {

				printf("0x%x(%d) Pri %d Pol %d T %d BP %d\n",
				       (int) pstate, (int) pstate->tid,
				       pstate->priority, pstate->policy,
				       pstate->ticks, pstate->base_priority);
			}
		}
	}
}

/*
 * This is the scheduler loop.
 */
void *
fixedpri_schedloop(void *arg)
{
	fixedpri_sched_t	*psched = (fixedpri_sched_t *) arg;
	schedmsg_t		msg;
	fixedpri_state_t	*current_thread = 0, *pstate = 0;
	sched_wakecond_t	wakeup_cond = 0;
	int			pedantic, rc;
	oskit_s32_t		timeout;
	pthread_t		tid;
#ifdef  MEASURE
	unsigned long		before, after;

	atexit(dump_fixedpri_stats);
#endif
	psched->tid = pthread_self();

	/*
	 * Must tell the main scheduling code ...
	 */
	pthread_sched_become_scheduler();

	/*
	 * Cancelation cleanup handler to cleanup resources at exit.
	 */
	pthread_cleanup_push(fixedpri_canceled, (void *) psched);

	/*
	 * Preemption means donate with non-zero timeout.
	 */
	if (psched->preemptible)
		timeout = PTHREAD_TICK;
	else
		timeout = 0;

	/*
	 * Root scheduler! Schedule the mainthread so it gets run.
	 */
	if (psched->maintid) {
		pstate = fixedpri_getstate(psched, psched->maintid);
		runq_insert_tail(psched, pstate);
		pedantic = 1;
	}
	else
		pedantic = fixedpri_pedantic;

	psched->ready = 1;

	CPUDEBUG(TRUE,
		 "fixedpri_schedloop(%d): ready - psched:0x%x\n",
		 psched->tid, (int) psched);

	while (1) {
		pthread_testcancel();
		
		CPUDEBUG(FIXEDPRI,
			 "fixedpri_schedloop(%d): qcount(%d) qmask(%x)\n",
			 psched->tid,
			 psched->runq_count, (int) psched->runq_mask);
		
		/*
		 * Consume any pending messages until there are no more.
		 */
		if (! pthread_sched_message_recv(&msg, 0))
			goto consume;

		if (CPUDEBUG_ISSET(FIXEDPRI))
			runq_check(psched);

		/*
		 * Find a thread to run.
		 */
		if (runq_empty(psched))
			current_thread = NULL_FPSTATE;
		else
			current_thread = runq_dequeue(psched);

		/*
		 * If we found a thread, switch into it and wait for
		 * a message. Otherwise, wait for messages to arrive
		 * that indicate something has changed.
		 */
		if (current_thread) {
			CPUDEBUG(FIXEDPRI,
				 "fixedpri_schedloop(%d): pstate 0x%x(%d)\n",
				 psched->tid, (int) current_thread,
				 current_thread->tid);
			
			if (pedantic || !runq_empty(psched))
				wakeup_cond = WAKEUP_ON_BLOCK;
			else
				wakeup_cond = WAKEUP_ON_SWITCH;

			/*
			 * Donate and check the return condition. 
			 */
			rc =
			 pthread_sched_donate_wait_recv(current_thread->tid,
					wakeup_cond, &msg, timeout);

			CPUDEBUG(FIXEDPRI,
				 "fixedpri_schedloop(%d): Donated: %d %s\n",
				 psched->tid, current_thread->tid,
				 msg_sched_rcodes[rc & ~SCHEDULE_MSGRECV]);
#ifdef  DEBUG
			current_thread->lastcode = rc;
#endif

			switch (rc & ~SCHEDULE_MSGRECV) {
			case SCHEDULE_NOTREADY:
				/*
				 * Thread was not ready to recv donation.
				 * Forget about this thread.
				 */
				break;
				
			case SCHEDULE_BLOCKED:
				/* Thread blocked, so forget about it. */
				break;
				
			case SCHEDULE_YIELDED:
				/* user specified yield */
				assert(! runq_onrunq(psched, current_thread));
				runq_insert_tail(psched, current_thread);
				break;
				
			case SCHEDULE_PREEMPTED:
				/* system directed yield */
				assert(! runq_onrunq(psched, current_thread));
				runq_insert_head(psched, current_thread);
				break;
				
			case SCHEDULE_TIMEDOUT:
				/* Time based preemption */
				pstate = current_thread;
				
				if (pstate->policy == SCHED_RR) {
					if (--pstate->ticks == 0) {
					     runq_insert_tail(psched, pstate);
					     pstate->ticks = SCHED_RR_INTERVAL;
					}
					else
					     runq_insert_head(psched, pstate);
				}
				else if (pstate->policy == SCHED_FIFO) {
					runq_insert_head(psched, pstate);
				}
				break;

			default:
				if (rc == SCHEDULE_MSGRECV)
					runq_insert_head(psched,
							 current_thread);
				else
					panic("fixedpri_schedloop: "
					      "Bad return code:%d", rc);
				break;
			}

			/* Back to the top to try again. */
			if (! (rc & SCHEDULE_MSGRECV))
				continue;
		}
		else {
			/*
			 * No threads to run so block waiting for a message.
			 */
			CPUDEBUG(FIXEDPRI,
				 "fixedpri_schedloop(%d): Recv\n",psched->tid);

			rc = pthread_sched_message_recv(&msg, -1);

			if (rc == OSKIT_ECANCELED)
				pthread_exit((void *) PTHREAD_CANCELED);
		}

		/*
		 * Process messages.
		 */
	consume:
		/* runq_check(psched); */

#ifdef  MEASURE
		before = get_tsc();
#endif
		/*
		 * Map tid in message to thread state structure. Avoid
		 * lookup if possible.
		 */
		assert(msg.tid);

		if (msg.type != MSG_SCHED_NEWTHREAD) {
			if (current_thread && msg.tid == current_thread->tid)
				pstate = current_thread;
			else
				pstate = fixedpri_getstate(psched, msg.tid);
		}

		CPUDEBUG(FIXEDPRI,
			 "fixedpri_schedloop(%d): MSG: %s %d 0x%x(%d)\n",
			 psched->tid,
			 msg_sched_typenames[msg.type], msg.opaque,
			 (int) pstate, (pstate ? pstate->tid : 0));

		switch (msg.type) {
		case MSG_SCHED_NEWTHREAD:
			/*
			 * New thread has joined us. Create a state structure
			 * and add it to the runq.
			 */
			pstate = fixedpri_thread_init(psched, msg.tid,
						      msg.opaque, msg.opaque2);

			/* and add it to the runq */
			runq_insert_tail(psched, pstate);
			break;
			
		case MSG_SCHED_UNBLOCK:
			/*
			 * A thread wants to be rescheduled. Place at the
			 * tail of the queue as defined by POSIX.
			 */
			if (! runq_onrunq(psched, pstate))
				runq_insert_tail(psched, pstate);

			break;
			
		case MSG_SCHED_SETSTATE:
			/*
			 * Thread priority has been changed.
			 */
			pstate->base_priority = (int) msg.opaque;
			
			if (runq_onrunq(psched, pstate)) {
				runq_remove(psched, pstate);
				pstate->priority = (int) msg.opaque;
				runq_insert_tail(psched, pstate);
			}
			else
				pstate->priority = (int) msg.opaque;
			break;

		case MSG_SCHED_EXITED:
			/*
			 * The thread has exited. 
			 */
			CPUDEBUG(FIXEDPRI,
				 "fixedpri_schedloop(%d): "
				 "Exit: Thread 0x%x(%d)\n",
				 pthread_self(), (int) pstate, pstate->tid);

			tid = pstate->tid;

			fixedpri_remstate(psched, tid);
			free(pstate);

			/*
			 * Special case is the main thread in the root
			 * scheduler. If that exits, the scheduler exits.
			 */
			if (psched->maintid && tid == psched->maintid)
				pthread_exit(0);
			    
			break;
		
		default:
			panic("fixedpri_schedloop: Bad message: %d 0x%x\n",
			      msg.type, pstate);
			break;
		}
#ifdef  MEASURE
		after = get_tsc();
		if (after > before) {
			stats.msgs++;
			stats.msg_cycles += (after - before);
		}
#endif
		
		/*runq_check(psched);*/
	}
	/*
	 * Never reached.
	 */
	pthread_cleanup_pop(1);
}

/*
 * Handle async cancel of the scheduler. Cleanup resources before the thread
 * disappears completely.
 *
 * XXX NOT CLEANING UP THREADS! What does it mean anyway to cancel a
 * scheduler before all its threads have been canceled? Also, there is an
 * obvious race condition that needs to be addressed; what happens if a
 * scheduler is canceled before its sees a particular thread for the first
 * time (and thus does not even know it exists as its child).
 */
void
fixedpri_canceled(void *arg)
{
	fixedpri_sched_t	*psched = (fixedpri_sched_t *) arg;

	CPUDEBUG(FIXEDPRI,
		 "fixedpri_terminate: Scheduler exiting:%d\n",
		 pthread_self());

	free(psched);
}

#ifdef	MEASURE
void
dump_fixedpri_stats()
{
	printf("FP msgs:        %d\n", stats.msgs);
	printf("FP msg Cycles:  %u\n", stats.msg_cycles);
}
#endif
#endif CPU_INHERIT
