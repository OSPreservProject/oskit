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
 * Simplified Lottery scheduler. Can be preemptive or not.
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
 * The runq is a single, doubly-linked queue, unordered.
 * TIDs are mapped back to the state structure using a key value.
 */
typedef struct lottosched {
	queue_head_t		runq;
	int			runq_count;
	int			tickets;	/* Total ticket count */
	oskit_u32_t		rstate[2];	/* Random number state */
	int			lastrand;
	short			ready;
	short			preemptible;
	pthread_t		tid;		/* Back pointer to thread */
	hash_table_t	       *hashtable;	/* Hash TID to pstate */
} lottosched_t;

/*
 * Per-thread scheduler state structure.
 */
typedef struct lotto_state {
	queue_chain_t	runq;			/* Queueing element */
	pthread_t	tid;			/* TID of thread */
	int		tickets;
} lotto_state_t;

#define NULL_LOTTOSTATE	((lotto_state_t *) 0)

/*
 * Local version of bad random number generator that stores state.
 */
static inline int
lotto_rand(lottosched_t *psched)
{
	psched->rstate[0] += 0xa859c317;
	psched->rstate[0] += (psched->rstate[1] << 13) |
		             (psched->rstate[1] >> 19);
	psched->rstate[1] += psched->rstate[0];
		
	return psched->lastrand = (psched->rstate[0] % psched->tickets);
}

static inline void
lotto_srand(lottosched_t *psched)
{
	psched->rstate[0] = psched->rstate[1] = (oskit_u32_t) psched;
}

extern int		ffs();
void		       *lotto_schedloop(void *arg);
static void		lotto_canceled(void *arg);
static int		lotto_pedantic = 1;

/*
 * Map TID to pstate structure with a hash table.
 */
static void
lotto_setstate(lottosched_t *psched, pthread_t tid, lotto_state_t *pstate)
{
	if (tidhash_add(psched->hashtable, (void *) pstate, tid))
		panic("lotto_setstate: "
		      "hashtable add failed: tid:%d pstate:0x%x",
		      tid, (int) pstate);
}

static lotto_state_t *
lotto_getstate(lottosched_t *psched, pthread_t tid)
{
	lotto_state_t	*pstate;

	if ((pstate = (lotto_state_t *)
	     tidhash_lookup(psched->hashtable, tid)) == NULL)
		panic("lotto_getstate: "
		      "hashtable lookup failed: tid:%d", tid);

	return pstate;
}

static void
lotto_remstate(lottosched_t *psched, pthread_t tid)
{
	tidhash_rem(psched->hashtable, tid);
}

/*
 * Create a Lotto scheduler. This creates the thread and makes sure it
 * gets run so that it exists and is ready to handle scheduling messages.
 */
int
create_lotto_scheduler(pthread_t *tid,
			const pthread_attr_t *attr, int preemptible)
{
	lottosched_t	*psched;

	if ((psched = (lottosched_t *) calloc(1, sizeof(lottosched_t)))== NULL)
		panic("create_lotto_scheduler: No more memory");

	if (tidhash_create(&psched->hashtable, 0))
		panic("allocate_lotto_scheduler: Hash Table allocation");
		
	psched->preemptible = preemptible;
	queue_init(&psched->runq);

	pthread_create(tid, attr, lotto_schedloop, (void *) psched);

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
lotto_state_t *
lotto_thread_init(lottosched_t *psched, pthread_t tid, int tickets)
{
	lotto_state_t	*pstate;

	if ((pstate = (lotto_state_t *) calloc(1, sizeof(lotto_state_t)))
	    == NULL)
		panic("lotto_thread_init: No more memory");

	pstate->tid      = tid;
	pstate->tickets  = tickets;
	lotto_setstate(psched, tid, pstate);

	return pstate;
}	

/*
 * Are there any threads on the runq?
 */
static inline int
runq_empty(lottosched_t *psched)
{
	return (psched->runq_count == 0);
}

/*
 * Determine if a pthread is on the runq. Use a separate field 
 * since using the flags would require locking the thread. Use the
 * queue chain pointer instead, setting it to zero when a thread is
 * removed from the queue.
 */
static inline int
runq_onrunq(lottosched_t *psched, lotto_state_t *pstate)
{
	return (int) pstate->runq.next;
}

/*
 * Add and remove threads from the runq.
 */

/*
 * Insert into the runq. The runq is ordered.
 */
static void
runq_insert(lottosched_t *psched, lotto_state_t *pstate)
{
	queue_head_t	*phdr = &(psched->runq);
	lotto_state_t	*ptmp;

	if (queue_empty(phdr)) {
		queue_enter(phdr, pstate, lotto_state_t *, runq);
		goto done;
	}
	
	queue_iterate(&(psched->runq), ptmp, lotto_state_t *, runq) {
		if (pstate->tickets >= ptmp->tickets) {
			queue_enter_before(phdr, ptmp,
					  pstate, lotto_state_t *, runq);
			goto done;
		}
	}

	queue_enter(phdr, pstate, lotto_state_t *, runq);
   done:
	psched->runq_count++;
	psched->tickets += pstate->tickets;
}

/*
 * Dequeue the highest priority thread. In this scheduler, that means
 * holding a lottery by picking a random ticket and searching the list
 * until the winner is found (cumulative ticket count reaches the winning
 * ticket number.
 */
static lotto_state_t *
runq_dequeue(lottosched_t *psched)
{
	queue_head_t		*phdr = &(psched->runq);
	lotto_state_t		*pnext;
	int			count, winning_ticket;

	count = 0;
	winning_ticket = lotto_rand(psched);
	queue_iterate(phdr, pnext, lotto_state_t *, runq) {
		count += pnext->tickets;

		if (count > winning_ticket)
			break;
	}
	
	queue_remove(phdr, pnext, lotto_state_t *, runq);
	pnext->runq.next = (queue_entry_t) 0;	
	psched->runq_count--;
	psched->tickets -= pnext->tickets;

	return pnext;
}

/*
 * Remove an arbitrary thread from the runq.
 */
static inline void
runq_remove(lottosched_t *psched, lotto_state_t *pstate)
{
	queue_head_t	*phdr = &(psched->runq);

	queue_remove(phdr, pstate, lotto_state_t *, runq);
	pstate->runq.next = (queue_entry_t) 0;	
	psched->runq_count--;
	psched->tickets -= pstate->tickets;
}

/*
 * Debug the runq.
 */
static void
runq_check(lottosched_t *psched)
{
	int		count, tickets;
	lotto_state_t	*pstate;
	
	tickets = count = 0;
	queue_iterate(&(psched->runq), pstate, lotto_state_t *, runq) {
		count++;
		tickets += pstate->tickets;
		if (count > psched->runq_count)
			panic("lotto scheduler: Bad runq(%d): 0x%x\n",
			      pthread_self(), psched);
	}
	if (tickets != psched->tickets)
		panic("lotto scheduler: Bad ticket count(%d): 0x%x\n",
		      pthread_self(), psched);
}

/*
 * Debug
 */
static void
lotto_debug(lottosched_t *psched)
{
	lotto_state_t	*pstate;

	printf("lotto(%d): Tickets %d Last: %d\n",
	       (int) pthread_self(), psched->tickets, psched->lastrand);
	
	queue_iterate(&(psched->runq), pstate, lotto_state_t *, runq) {
		printf("0x%x(%d) T %d\n",
		       (int) pstate, (int) pstate->tid, pstate->tickets);
	}
}

/*
 * This is the scheduler loop.
 */
void *
lotto_schedloop(void *arg)
{
	lottosched_t		*psched = (lottosched_t *) arg;
	schedmsg_t		msg;
	lotto_state_t		*current_thread = 0, *pstate = 0;
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
	pthread_cleanup_push(lotto_canceled, (void *) psched);

	pedantic = lotto_pedantic;
	psched->ready = 1;
	lotto_srand(psched);

	CPUDEBUG(LOTTO,
		 "lotto_schedloop(%d): ready - psched:0x%x\n",
		 psched->tid, (int) psched);

	while (1) {
		pthread_testcancel();

		CPUDEBUG(LOTTO,
			 "lotto_schedloop(%d): qcount(%d)\n",
			 psched->tid, psched->runq_count);
		
		/*
		 * Consume any pending messages until there are no more.
		 */
		if (! pthread_sched_message_recv(&msg, 0))
			goto consume;

		if (CPUDEBUG_ISSET(LOTTO))
			lotto_debug(psched);
		
		/*
		 * Find a thread to run.
		 */
		if (runq_empty(psched))
			current_thread = NULL_LOTTOSTATE;
		else
			current_thread = runq_dequeue(psched);

		/*
		 * If we found a thread, switch into it and wait for
		 * a message. Otherwise, wait for messages to arrive
		 * that indicate something has changed.
		 */
		if (current_thread) {
			CPUDEBUG(LOTTO,
				 "lotto_schedloop(%d): pstate 0x%x(%d)\n",
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

			CPUDEBUG(LOTTO,
				 "lotto_schedloop(%d): Donated: %d %s\n",
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
					panic("lotto_schedloop: "
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
			CPUDEBUG(LOTTO,
				 "lotto_schedloop(%d): Recv\n", psched->tid);
			
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
				pstate = lotto_getstate(psched, msg.tid);
		}

		CPUDEBUG(LOTTO,
			 "lotto_schedloop(%d): Message: %s 0x%x(%d)\n",
			 psched->tid,
			 msg_sched_typenames[msg.type], (int) pstate,
			 (pstate ? pstate->tid : 0));

		switch (msg.type) {
		case MSG_SCHED_NEWTHREAD:
			/*
			 * New thread has joined us. Create a state structure
			 * and add it to the runq.
			 */
			pstate = lotto_thread_init(psched,
						   msg.tid, msg.opaque);

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
			 * value is is the thread period, which is the same
			 * as its priority in this scheduling model.
			 */
			if (runq_onrunq(psched, pstate)) {
				runq_remove(psched, pstate);
				pstate->tickets = (int) msg.opaque;
				runq_insert(psched, pstate);
			}
			else
				pstate->tickets = (int) msg.opaque;
			break;

		case MSG_SCHED_EXITED:
			/*
			 * The thread has exited. 
			 */
			CPUDEBUG(LOTTO,
				 "lotto_schedloop(%d): "
				 "Exit: Thread 0x%x(%d)\n",
				 pthread_self(), (int) pstate, pstate->tid);

			lotto_remstate(psched, pstate->tid);
			free(pstate);
			break;
		
		default:
			panic("lotto_schedloop: Bad message: %d 0x%x\n",
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
lotto_canceled(void *arg)
{
	lottosched_t	*psched = (lottosched_t *) arg;

	CPUDEBUG(LOTTO,
		 "lotto_terminate: Scheduler exiting:%d\n",
		 pthread_self());

	free(psched);
}
#endif CPU_INHERIT
