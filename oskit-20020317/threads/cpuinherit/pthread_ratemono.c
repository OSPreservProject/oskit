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
 * Simplified Rate Monotonic scheduler. Can be preemptive or not.
 */
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <oskit/threads/pthread.h>
#include <oskit/threads/cpuinherit.h>
#include <oskit/queue.h>
#include <malloc.h>
#include <assert.h>
#include "hash.h"

/*
 * The scheduler structures are per-scheduler instantiation.
 *
 * The runq is a single doubly linked queue, ordered by schedule period.
 * TIDs are mapped back to the state structure using a key value.
 */
typedef struct rmsched {
	queue_head_t		runq;		/* Runq */
	int			runq_count;	/* Number of ready threads */
	short			ready;		/* Scheduler is ready to go */
	short			preemptible;	/* Scheduler is preemptive */
	pthread_t	        tid;	        /* Back pointer to thread */
	hash_table_t	       *hashtable;	/* Hash TID to pstate */
} rmsched_t;

/*
 * Per-thread state information structure.
 */
typedef struct rm_state {
	queue_chain_t	runq;			/* Queueing element */
	pthread_t	tid;			/* TID of thread */
	int		period;			/* In ms. */
	int		duration;		/* In ms. */
} rm_state_t;

#define NULL_RMSTATE	((rm_state_t *) 0)

extern int		ffs();
void		       *ratemono_schedloop(void *arg);
static void		ratemono_canceled(void *arg);
static int		ratemono_pedantic = 1;

/*
 * Map TID to pstate structure with a hash table.
 */
static void
ratemono_setstate(rmsched_t *psched, pthread_t tid, rm_state_t *pstate)
{
	if (tidhash_add(psched->hashtable, (void *) pstate, tid))
		panic("ratemono_setstate: "
		      "hashtable add failed: tid:%d pstate:0x%x",
		      tid, (int) pstate);
}

static rm_state_t *
ratemono_getstate(rmsched_t *psched, pthread_t tid)
{
	rm_state_t	*pstate;

	if ((pstate = (rm_state_t *)
	     tidhash_lookup(psched->hashtable, tid)) == NULL)
		panic("ratemono_getstate: "
		      "hashtable lookup failed: tid:%d", tid);

	return pstate;
}

static void
ratemono_remstate(rmsched_t *psched, pthread_t tid)
{
	tidhash_rem(psched->hashtable, tid);
}

/*
 * Create a Rate Monotonic scheduler. This creates the thread and makes sure
 * it gets run so that it exists and is ready to handle scheduling
 * messages.
 */
int
create_ratemono_scheduler(pthread_t *tid,
			  const pthread_attr_t *attr, int preemptible)
{
	rmsched_t	*psched;

	if ((psched = (rmsched_t *) calloc(1, sizeof(rmsched_t))) == NULL)
		panic("create_ratemono_scheduler: No more memory");

	if (tidhash_create(&psched->hashtable, 0))
		panic("create_ratemono_scheduler: Hash Table allocation");
		
	psched->preemptible = preemptible;
	queue_init(&psched->runq);

	pthread_create(tid, attr, ratemono_schedloop, (void *) psched);

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
 * Initialize the per-thread scheduler state structure. This is called
 * when a new thread is created. The state structure is then stored
 * in the key table so that TIDs can be mapped back to the state structure.
 */
rm_state_t *
ratemono_thread_init(rmsched_t *psched, pthread_t tid, int period)
{
	rm_state_t	*pstate;

	if ((pstate = (rm_state_t *) calloc(1, sizeof(rm_state_t))) == NULL)
		panic("ratemono_thread_init: No more memory");

	pstate->tid          = tid;
	pstate->period       = period;
	ratemono_setstate(psched, tid, pstate);

	return pstate;
}	

/*
 * Are there any threads on the runq?
 */
static inline int
runq_empty(rmsched_t *psched)
{
	return (psched->runq_count == 0);
}

/*
 * Determine if a thread is on the runq. Use the runq.next pointer since
 * its not doing much else.
 */
static inline int
runq_onrunq(rmsched_t *psched, rm_state_t *pstate)
{
	return (int) pstate->runq.next;
}

/*
 * Add and remove threads from the runq.
 */

/*
 * Insert into the runq. The queue is ordered by the scheduling period,
 * with lowest at the beginning.
 */
static void
runq_insert(rmsched_t *psched, rm_state_t *pstate)
{
	queue_head_t	*phdr = &(psched->runq);
	rm_state_t	*ptmp;

	if (queue_empty(phdr)) {
		queue_enter(phdr, pstate, rm_state_t *, runq);
		goto done;
	}
	
	queue_iterate(&(psched->runq), ptmp, rm_state_t *, runq) {
		if (pstate->period <= ptmp->period) {
			queue_enter_before(phdr, ptmp,
					   pstate, rm_state_t *, runq);
			goto done;
		}
	}
	queue_enter(phdr, pstate, rm_state_t *, runq);
   done:
	psched->runq_count++;
}

/*
 * Dequeue highest priority thread, which is actually the first thread
 * on the runq.
 */
static rm_state_t *
runq_dequeue(rmsched_t *psched)
{
	queue_head_t	*phdr = &(psched->runq);
	rm_state_t	*pnext;

	queue_remove_first(phdr, pnext, rm_state_t *, runq);
	pnext->runq.next = (queue_entry_t) 0;
	psched->runq_count--;

	return pnext;
}

/*
 * Remove an arbitrary thread from the runq.
 */
static inline void
runq_remove(rmsched_t *psched, rm_state_t *pstate)
{
	queue_head_t	*phdr = &(psched->runq);

	queue_remove(phdr, pstate, rm_state_t *, runq);
	pstate->runq.next = (queue_entry_t) 0;	
	psched->runq_count--;
}

/*
 * Debug the runq.
 */
static void
runq_check(rmsched_t *psched)
{
	int		count;
	rm_state_t	*pstate;
	
	count = 0;
	queue_iterate(&(psched->runq), pstate, rm_state_t *, runq) {
		count++;
		if (count > psched->runq_count)
			panic("ratemono scheduler: Bad runq(%d): 0x%x\n",
			      pthread_self(), psched);
	}
}

/*
 * Debug
 */
static void
rmono_debug(rmsched_t *psched)
{
	rm_state_t	*pstate;

	printf("ratemono(%d):\n", (int) pthread_self());
	
	queue_iterate(&(psched->runq), pstate, rm_state_t *, runq) {
		printf("0x%x(%d) Period %d\n",
		       (int) pstate, (int) pstate->tid, pstate->period);
	}
}

/*
 * This is the scheduler loop.
 */
void *
ratemono_schedloop(void *arg)
{
	rmsched_t		*psched = (rmsched_t *) arg;
	schedmsg_t		msg;
	rm_state_t		*current_thread = 0, *pstate = 0;
	sched_wakecond_t	wakeup_cond = 0;
	int			pedantic, rc;
	oskit_s32_t		timeout;

	psched->tid = pthread_self();

	/*
	 * Must tell the main scheduling code ...
	 */
	pthread_sched_become_scheduler();

	/*
	 * Preemption means donate with non-zero timeout.
	 */
	if (psched->preemptible)
		timeout = PTHREAD_TICK;
	else
		timeout = 0;

	/*
	 * Cancelation cleanup handler to cleanup resources at exit.
	 */
	pthread_cleanup_push(ratemono_canceled, (void *) psched);

	pedantic = ratemono_pedantic;
	psched->ready = 1;

	while (1) {
		pthread_testcancel();

		CPUDEBUG(RATEMONO,
			 "ratemono_schedloop(%d): qcount(%d)\n",
			 psched->tid, psched->runq_count);
		
		/*
		 * Consume any pending messages until there are no more.
		 */
		if (! pthread_sched_message_recv(&msg, 0))
			goto consume;

		if (CPUDEBUG_ISSET(RATEMONO))
			rmono_debug(psched);

		/*
		 * Find a thread to run.
		 */
		if (runq_empty(psched))
			current_thread = NULL_RMSTATE;
		else
			current_thread = runq_dequeue(psched);

		/*
		 * If we found a thread, switch into it and wait for
		 * a message. Otherwise, wait for messages to arrive
		 * that indicate something has changed.
		 */
		if (current_thread) {
			CPUDEBUG(RATEMONO,
				 "ratemono_schedloop(%d): pstate 0x%x(%d)\n",
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

			CPUDEBUG(RATEMONO,
				 "ratemono_schedloop(%d): Donated: %d %s\n",
				 psched->tid, current_thread->tid,
				 msg_sched_rcodes[rc & ~SCHEDULE_MSGRECV]);

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
				
			case SCHEDULE_PREEMPTED:
			case SCHEDULE_YIELDED:
			case SCHEDULE_TIMEDOUT:
				assert(! runq_onrunq(psched, current_thread));
				runq_insert(psched, current_thread);
				break;

			default:
				if (rc == SCHEDULE_MSGRECV)
					runq_insert(psched, current_thread);
				else
					panic("ratemono_schedloop: "
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
			CPUDEBUG(RATEMONO,
				 "ratemono_schedloop(%d): Recv\n",psched->tid);
			
			rc = pthread_sched_message_recv(&msg, -1);

			if (rc == OSKIT_ECANCELED)
				pthread_exit((void *) PTHREAD_CANCELED);
		}

		/*
		 * Process messages.
		 */
	consume:
		/*
		 * Map tid in message to thread state structure. Avoid
		 * lookup if possible.
		 */
		assert(msg.tid);

		if (msg.type != MSG_SCHED_NEWTHREAD) {
			if (current_thread && msg.tid == current_thread->tid)
				pstate = current_thread;
			else
				pstate = ratemono_getstate(psched, msg.tid);
		}

		CPUDEBUG(RATEMONO,
			 "ratemono_schedloop(%d): Message: %s 0x%x(%d)\n",
			 psched->tid,
			 msg_sched_typenames[msg.type], (int) pstate,
			 (pstate ? pstate->tid : 0));

		switch (msg.type) {
		case MSG_SCHED_NEWTHREAD:
			/*
			 * New thread has joined us. Create a state structure
			 * and add it to the runq.
			 */
			pstate = ratemono_thread_init(psched, msg.tid,
						      msg.opaque);

			/* and add it to the runq */
			runq_insert(psched, pstate);
			break;
			
		case MSG_SCHED_UNBLOCK:
			/*
			 * A thread wants to be rescheduled.
			 */
			if (! runq_onrunq(psched, pstate))
				runq_insert(psched, pstate);
			break;
			
		case MSG_SCHED_SETSTATE:
			/*
			 * Thread parameters have been changed. The opaque
			 * value is the thread period, which is the same
			 * as its priority in this scheduling model.
			 */
			if (runq_onrunq(psched, pstate)) {
				runq_remove(psched, pstate);
				pstate->period = (int) msg.opaque;
				runq_insert(psched, pstate);
			}
			else
				pstate->period = (int) msg.opaque;

			break;

		case MSG_SCHED_EXITED:
			/*
			 * The thread has exited. 
			 */
			CPUDEBUG(RATEMONO,
				 "ratemono_schedloop(%d): "
				 "Exit: Thread 0x%x(%d)\n",
				 pthread_self(), (int) pstate, pstate->tid);

			ratemono_remstate(psched, pstate->tid);
			free(pstate);
			break;
		
		default:
			panic("ratemono_schedloop: Bad message: %d 0x%x\n",
			      msg.type, pstate);
			break;
		}
		runq_check(psched);
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
 * XXX NOT CLEANING UP THREADS!
 */
void
ratemono_canceled(void *arg)
{
	rmsched_t	*psched = (rmsched_t *) arg;

	CPUDEBUG(RATEMONO,
		 "ratemono_terminate: Scheduler exiting:%d\n",
		 pthread_self());

	free(psched);
}
#endif CPU_INHERIT
