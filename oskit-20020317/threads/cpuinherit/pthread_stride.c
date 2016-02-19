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
 * Simplified Stride scheduler. Can be preemptive or not.
 */
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <oskit/threads/pthread.h>
#include <oskit/threads/cpuinherit.h>
#include <oskit/queue.h>
#include <malloc.h>
#include <assert.h>
#include <oskit/x86/pc/pit.h>
#include "hash.h"

/*
 * The scheduler structures are per-scheduler instantiation.
 *
 * The runq is a single, doubly-linked queue.
 */
typedef struct stridesched {
	queue_head_t		runq;
	int			runq_count;
	int			tickets;	/* Global tickets */
	int			pass;		/* Global pass */
	int			stride;		/* Global stride */
	int			last_update;
	short			ready;
	short			preemptible;
	pthread_t		tid;		/* Back pointer to thread */
	hash_table_t	       *hashtable;	/* Hash TID to pstate */
} stridesched_t;

#define BASE_TICKETS	(1024 * 4)
#define STRIDE1		(1 << 15)
#define QUANTUM		((PIT_HZ/100) / 1000)

/*
 * Per-thread scheduler state structure.
 */
typedef struct stride_state {
	queue_chain_t	runq;			/* Queueing element */
	pthread_t	tid;			/* TID of thread */
	int		tickets;
	int		stride;
	int		pass;
	int		remain;
	int		start;
} stride_state_t;

#define NULL_STRIDESTATE ((stride_state_t *) 0)
#define TICKETS(p)	(p->tickets)
#define STRIDE(p)	(p->stride)
#define PASS(p)		(p->pass)
#define REMAIN(p)	(p->remain)
#define START(p)	(p->start)

extern int		ffs();
void		       *stride_schedloop(void *arg);
static void		stride_canceled(void *arg);
static int		stride_pedantic = 1;
static void		client_init(stride_state_t *pstate, int tickets);

/*
 * Map TID to pstate structure with a hash table.
 */
static void
stride_setstate(stridesched_t *psched, pthread_t tid, stride_state_t *pstate)
{
	if (tidhash_add(psched->hashtable, (void *) pstate, tid))
		panic("stride_setstate: "
		      "hashtable add failed: tid:%d pstate:0x%x",
		      tid, (int) pstate);
}

static stride_state_t *
stride_getstate(stridesched_t *psched, pthread_t tid)
{
	stride_state_t	*pstate;

	if ((pstate = (stride_state_t *)
	     tidhash_lookup(psched->hashtable, tid)) == NULL)
		panic("stride_getstate: "
		      "hashtable lookup failed: tid:%d", tid);

	return pstate;
}

static void
stride_remstate(stridesched_t *psched, pthread_t tid)
{
	tidhash_rem(psched->hashtable, tid);
}

/*
 * Create a Stride scheduler. This creates the thread and makes sure it
 * gets run so that it exists and is ready to handle scheduling messages.
 */
int
create_stride_scheduler(pthread_t *tid,
			const pthread_attr_t *attr, int preemptible)
{
	stridesched_t	*psched;

	if ((psched = (stridesched_t *) calloc(1, sizeof(stridesched_t)))
	    == NULL)
		panic("create_stride_scheduler: No more memory");

	if (tidhash_create(&psched->hashtable, 0))
		panic("allocate_stride_scheduler: Hash Table allocation");
		
	psched->preemptible = preemptible;
	queue_init(&psched->runq);

	pthread_create(tid, attr, stride_schedloop, (void *) psched);

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
 * when a new thread is created, and is stashed in the parent scheduler
 * thread structure.
 */
stride_state_t *
stride_thread_init(stridesched_t *psched, pthread_t tid, int tickets)
{
	stride_state_t	*pstate;

	if ((pstate = (stride_state_t *) calloc(1, sizeof(stride_state_t)))
	    == NULL)
		panic("stride_thread_init: No more memory");

	pstate->tid = tid;
	stride_setstate(psched, tid, pstate);
	client_init(pstate, tickets);

	return pstate;
}	

/*
 * Runq stuff
 */
/*
 * Are there any threads on the runq?
 */
static inline int
runq_empty(stridesched_t *psched)
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
runq_onrunq(stridesched_t *psched, stride_state_t *pstate)
{
	return (int) pstate->runq.next;
}

/*
 * Add and remove threads from the runq.
 */

/*
 * Insert into the runq.
 */
static void
runq_insert(stridesched_t *psched, stride_state_t *pstate)
{
	queue_head_t	*phdr = &(psched->runq);
	stride_state_t	*ptmp;

	if (queue_empty(phdr)) {
		queue_enter(phdr, pstate, stride_state_t *, runq);
		goto done;
	}
	
	queue_iterate(&(psched->runq), ptmp, stride_state_t *, runq) {
		if (PASS(pstate) <= PASS(ptmp)) {
			queue_enter_before(phdr, ptmp,
					  pstate, stride_state_t *, runq);
			goto done;
		}
	}
	queue_enter(phdr, pstate, stride_state_t *, runq);
   done:
	psched->runq_count++;
}

/*
 * Dequeue the highest priority thread, which is the first thread since
 * the list ordered by PASS.
 */
static stride_state_t *
runq_dequeue(stridesched_t *psched)
{
	queue_head_t	*phdr = &(psched->runq);
	stride_state_t	*pnext;

	queue_remove_first(phdr, pnext, stride_state_t *, runq);
	pnext->runq.next = (queue_entry_t) 0;	
	psched->runq_count--;
	START(pnext) = oskit_pthread_childtime(pnext->tid);

	return pnext;
}

/*
 * Remove an arbitrary thread from the runq.
 */
static inline void
runq_remove(stridesched_t *psched, stride_state_t *pstate)
{
	queue_head_t	*phdr = &(psched->runq);

	queue_remove(phdr, pstate, stride_state_t *, runq);
	pstate->runq.next = (queue_entry_t) 0;	
	psched->runq_count--;
}

/*
 * Debug the runq.
 */
static void
runq_check(stridesched_t *psched)
{
	int		count, tickets;
	stride_state_t	*pstate;
	
	tickets = count = 0;
	queue_iterate(&(psched->runq), pstate, stride_state_t *, runq) {
		count++;
		tickets += TICKETS(pstate);
		if (count > psched->runq_count)
			panic("stride scheduler: Bad runq(%d): 0x%x\n",
			      pthread_self(), psched);
	}
	if (tickets != psched->tickets)
		panic("stride scheduler: Bad ticket count(%d): 0x%x\n",
		      pthread_self(), psched);
}

/*
 * Stride scheduler specific routines.
 */

/*
 * Update global pass based on elapsed real time.
 */
static void
global_pass_update(stridesched_t *psched)
{
	int	elapsed = oskit_pthread_childtime(psched->tid) -
			  psched->last_update;

	psched->pass += (psched->stride * elapsed) / QUANTUM;
	psched->last_update += elapsed;
}

static void
thread_pass_update(stridesched_t *psched, stride_state_t *pstate)
{
	int	elapsed;

	elapsed  = oskit_pthread_childtime(pstate->tid);
	elapsed -= START(pstate);

	PASS(pstate) += (STRIDE(pstate) * elapsed) / QUANTUM;
}

/*
 * Update global tickets and stride to reflect change in runq.
 */
static void 
global_tickets_update(stridesched_t *psched, int delta)
{
	psched->tickets += delta;

	/*
	 * XXX: Is this the correct thing to do when the last thread
	 * leaves? The paper says nothing about it.
	 */
	if (psched->tickets)
		psched->stride = STRIDE1 / psched->tickets;
	else
		psched->stride = STRIDE1;
}

/*
 * Initialize the tickets for a new thread to some reasonable value.
 */
static void
client_init(stride_state_t *pstate, int tickets)
{
	TICKETS(pstate) = tickets;
	STRIDE(pstate)  = STRIDE1 / tickets;
	REMAIN(pstate)  = STRIDE(pstate);
}

/*
 * Join the party.
 */
static void
client_join(stridesched_t *psched, stride_state_t *pstate)
{
	global_pass_update(psched);
	PASS(pstate) = psched->pass + REMAIN(pstate);
	global_tickets_update(psched, TICKETS(pstate));
	runq_insert(psched, pstate);
}

/*
 * Leave the party
 */
static void
client_leave(stridesched_t *psched, stride_state_t *pstate)
{
	global_pass_update(psched);
	REMAIN(pstate) = PASS(pstate) - psched->pass;
	global_tickets_update(psched, -TICKETS(pstate));
}

/*
 * Change priority
 */
static void
client_modify(stridesched_t *psched, stride_state_t *pstate,
	      int current, int tickets)
{
	int	remain, stride, queued;

	if ((queued = runq_onrunq(psched, pstate))) {
		runq_remove(psched, pstate);
		client_leave(psched, pstate);
	}
	else if (current)
		global_tickets_update(psched, -TICKETS(pstate));

	stride = STRIDE1 / tickets;
	remain = (REMAIN(pstate) * stride) / STRIDE(pstate);

	TICKETS(pstate) = tickets;
	STRIDE(pstate)  = stride;
	REMAIN(pstate)  = remain;

	if (queued)
		client_join(psched, pstate);
	else if (current)
		global_tickets_update(psched, TICKETS(pstate));
}

/*
 * Debug
 */
static void
stride_debug(stridesched_t *psched)
{
	stride_state_t	*pstate;

	printf("stride(%d): GT %d GS %d GP %d LU %d\n",
	       (int) pthread_self(),
	       psched->tickets, psched->stride, psched->pass,
	       psched->last_update);

	
	queue_iterate(&(psched->runq), pstate, stride_state_t *, runq) {
		printf("0x%x(%d) T %d S %d P %d R %d S %d\n",
		       (int) pstate, (int) pstate->tid,
		       TICKETS(pstate), STRIDE(pstate), PASS(pstate),
		       REMAIN(pstate), START(pstate));
	}
}

/*
 * This is the scheduler loop.
 */
void *
stride_schedloop(void *arg)
{
	stridesched_t		*psched = (stridesched_t *) arg;
	schedmsg_t		msg;
	stride_state_t		*current_thread = 0, *pstate = 0;
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
	pthread_cleanup_push(stride_canceled, (void *) psched);

	pedantic = stride_pedantic;
	psched->ready = 1;

	CPUDEBUG(STRIDE,
		 "stride_schedloop(%d): ready - psched:0x%x\n",
		 psched->tid, (int) psched);

	while (1) {
		pthread_testcancel();
		
		CPUDEBUG(STRIDE,
			 "stride_schedloop(%d): qcount(%d)\n",
			 psched->tid, psched->runq_count);
		
		/*
		 * Consume any pending messages until there are no more.
		 */
		if (! pthread_sched_message_recv(&msg, 0))
			goto consume;

		runq_check(psched);
		if (CPUDEBUG_ISSET(STRIDE))
			stride_debug(psched);
	
		/*
		 * Find a thread to run.
		 */
		if (runq_empty(psched))
			current_thread = NULL_STRIDESTATE;
		else
			current_thread = runq_dequeue(psched);

		/*
		 * If we found a thread, switch into it and wait for
		 * a message. Otherwise, wait for messages to arrive
		 * that indicate something has changed.
		 */
		if (current_thread) {
			CPUDEBUG(STRIDE,
				 "stride_schedloop(%d): pstate 0x%x(%d)\n",
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

			CPUDEBUG(STRIDE,
				 "stride_schedloop(%d): Donated: %d %s\n",
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
				client_leave(psched, current_thread);
				break;
				
			case SCHEDULE_PREEMPTED:
			case SCHEDULE_YIELDED:
			case SCHEDULE_TIMEDOUT:
				assert(! runq_onrunq(psched, current_thread));
				thread_pass_update(psched, current_thread);
				runq_insert(psched, current_thread);
				break;
				
			default:
				if (rc == SCHEDULE_MSGRECV) {
					thread_pass_update(psched,
							   current_thread);
					runq_insert(psched, current_thread);
				}
				else
					panic("stride_schedloop: "
					      "Bad return code:%d", rc);
				break;
			}

			/* Back to the top to look for more messages */
			if (! (rc & SCHEDULE_MSGRECV))
				continue;
		}
		else {
			/*
			 * No threads to run so block waiting for a message.
			 */
			CPUDEBUG(STRIDE,
				 "stride_schedloop(%d): Recv\n", psched->tid);
			
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
				pstate = stride_getstate(psched, msg.tid);
		}

		CPUDEBUG(STRIDE,
			 "stride_schedloop(%d): Message: %s 0x%x(%d)\n",
			 psched->tid,
			 msg_sched_typenames[msg.type], (int) pstate,
			 (pstate ? pstate->tid : 0));

		switch (msg.type) {
		case MSG_SCHED_NEWTHREAD:
			/*
			 * New thread has joined us. Create a state structure
			 * and add it to the runq.
			 */
			pstate = stride_thread_init(psched, msg.tid,
						    msg.opaque);

			/* and add it to the runq */
			client_join(psched, pstate);
			break;
		
		case MSG_SCHED_UNBLOCK:
			/*
			 * A thread wants to be rescheduled. Might be the
			 * first time this thread was ever seen.
			 */
			if (! runq_onrunq(psched, pstate))
				client_join(psched, pstate);
			break;
			
		case MSG_SCHED_SETSTATE:
			/*
			 * Thread parameters have been changed. The opaque
			 * value is is the new tickets.
			 */
			client_modify(psched, pstate,
				      pstate == current_thread,
				      (int) msg.opaque);
			break;

		case MSG_SCHED_EXITED:
			/*
			 * The thread has exited. 
			 */
			CPUDEBUG(STRIDE,
				 "stride_schedloop(%d): "
				 "Exit: Thread 0x%x(%d)\n",
				 pthread_self(), (int) pstate, pstate->tid);

			stride_remstate(psched, pstate->tid);
			free(pstate);
			break;
		
		default:
			panic("stride_schedloop: Bad message: %d 0x%x\n",
			      msg.type, pstate);
			break;
		}
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
stride_canceled(void *arg)
{
	stridesched_t	*psched = (stridesched_t *) arg;

	CPUDEBUG(STRIDE,
		 "stride_terminate: Scheduler exiting:%d\n",
		 pthread_self());

	free(psched);
}
#endif CPU_INHERIT
