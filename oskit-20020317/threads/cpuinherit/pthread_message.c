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
#include <threads/pthread_internal.h>
#include <oskit/c/malloc.h>

/*
 * Allocate and initialize a scheduler message queue.
 */
schedmsg_queue_t *
schedmsg_queue_allocate(int size)
{
	int		  logsize;
	schedmsg_queue_t *queue;

	/*
	 * Must be a power of two, so round to next power of two greater
	 * than the given size.
	 */
	for (logsize = 1; logsize < size; logsize *= 2)
		;

	if ((queue = (schedmsg_queue_t *)
	     calloc(1, sizeof(schedmsg_queue_t) +
		    ((logsize - 1) * sizeof(schedmsg_t)))) == NULL)
		panic("schedmsg_queue_allocate: No more memory");

	queue->mask = logsize - 1;
	queue->size = logsize;
	pthread_lock_init(&queue->lock);

	return queue;
}

/*
 * Deallocate a scheduler message queue.
 */
void
schedmsg_queue_deallocate(schedmsg_queue_t *queue)
{
	free(queue);
}

/*
 * Check queue status.
 */
static inline int
schedmsg_queue_empty(schedmsg_queue_t *queue)
{
	int		empty = 0;

	if (queue->head == queue->tail)
		empty = 1;

	return empty;
}

static inline int
schedmsg_queue_full(schedmsg_queue_t *queue)
{
	int		full = 0;

	if ((queue->head - queue->tail) == queue->size)
		full = 1;

	return full;
}

/*
 * Enqueue a message for delivery.
 */
static inline void
schedmsg_enqueue(schedmsg_queue_t *queue, schedmsg_types_t type,
		 pthread_t tid, oskit_u32_t opaque, oskit_u32_t opaque2)
{
	int		x;

	x = (queue->head++ & queue->mask);
	queue->entries[x].type    = type;
	queue->entries[x].tid     = tid;
	queue->entries[x].opaque  = opaque;
	queue->entries[x].opaque2 = opaque2;
}

/*
 * Return the next available message. The values are placed in the provided
 * message buffer since the actual queue entry cannot be used.
 *
 * Return 0 if none available, 1 otherwise.
 */
static inline void
schedmsg_dequeue(schedmsg_queue_t *queue, schedmsg_t *msg)
{
	int		x;

	x = (queue->tail++ & queue->mask);
	msg->type    = queue->entries[x].type;
	msg->tid     = queue->entries[x].tid;
	msg->opaque  = queue->entries[x].opaque;
	msg->opaque2 = queue->entries[x].opaque2;
}

/*
 * Blocking receive. The receiver waits for a message to arrive. Timeout
 * determines how long the wait, but for now its never or forever.
 */
oskit_error_t
pthread_sched_message_recv(schedmsg_t *msg, oskit_s32_t timeout)
{
	pthread_thread_t	*pthread = CURPTHREAD();
	int			enabled;

	enabled = save_disable_interrupts();

	pthread_lock(&pthread->msgqueue->lock);
	if (!schedmsg_queue_empty(pthread->msgqueue)) {
		schedmsg_dequeue(pthread->msgqueue, msg);
		pthread_unlock(&pthread->msgqueue->lock);
		restore_interrupt_enable(enabled);
		return 0;
	}

	/*
	 * Only two timeout values right now, 0 and forever ...
	 */
	if (timeout == 0) {
		pthread_unlock(&pthread->msgqueue->lock);
		restore_interrupt_enable(enabled);
		return OSKIT_EAGAIN;
	}
	pthread_unlock(&pthread->msgqueue->lock);

	pthread_lock(&pthread->waitlock);
	pthread->waitflags |= THREAD_WS_CPUIRECV_WAIT;
	pthread_sched_reschedule(RESCHED_BLOCK, &pthread->waitlock);

	/*
	 * Back from receive. The sender woke us up, or we got canceled.
	 */
	pthread_lock(&pthread->lock);
	if (pthread->flags & THREAD_CANCELED) {
		pthread_unlock(&pthread->lock);
		restore_interrupt_enable(enabled);
		return OSKIT_ECANCELED;
	}
	pthread_unlock(&pthread->lock);

	/*
	 * Extract the message.
	 */
	pthread_lock(&pthread->msgqueue->lock);
	if (schedmsg_queue_empty(pthread->msgqueue))
		panic("pthread_sched_message_recv: No message!");
	schedmsg_dequeue(pthread->msgqueue, msg);
	pthread_unlock(&pthread->msgqueue->lock);

	restore_interrupt_enable(enabled);
	return 0;
}

/*
 * Non blocking scheduler message send.
 */
oskit_error_t
pthread_sched_message_send(pthread_thread_t *pthread, schedmsg_t *msg)
{
	int			enabled;

	enabled = save_disable_interrupts();

	/*
	 * Insert message into receiver queue.
	 */
	pthread_lock(&pthread->msgqueue->lock);
	if (schedmsg_queue_full(pthread->msgqueue))
		panic("pthread_sched_message_send: Message Queue Full!");
		
	schedmsg_enqueue(pthread->msgqueue,
			 msg->type, msg->tid, msg->opaque, msg->opaque2);
	
	pthread_unlock(&pthread->msgqueue->lock);

	/*
	 * If the target is in the wait, get it going again.
	 */
	pthread_lock(&pthread->waitlock);
	if (pthread->waitflags & THREAD_WS_CPUIRECV_WAIT) {
		pthread->waitflags &= ~THREAD_WS_CPUIRECV_WAIT;
		pthread_unlock(&pthread->waitlock);
		restore_interrupt_enable(enabled);
		pthread_sched_setrunnable(pthread);
		return 0;
	}
	pthread_unlock(&pthread->waitlock);
	
	restore_interrupt_enable(enabled);
	return 0;
}

/*
 * Special version for sending a scheduler message out of the wakeup code.
 *
 * The thread is locked. Interrupts are blocked.
 */
oskit_error_t
pthread_sched_special_send(pthread_thread_t *pthread, schedmsg_t *msg)
{
	assert_interrupts_disabled();
	/*
	 * Insert message into receiver queue.
	 */
	pthread_lock(&pthread->msgqueue->lock);

	if (schedmsg_queue_full(pthread->msgqueue))
		panic("pthread_sched_special_send: Message Queue Full!");
		
	schedmsg_enqueue(pthread->msgqueue,
			 msg->type, msg->tid, msg->opaque, msg->opaque2);

	pthread_unlock(&pthread->msgqueue->lock);

	/*
	 * If the target is in the wait, get it going again.
	 */
	if (pthread->waitflags & THREAD_WS_CPUIRECV_WAIT) {
		pthread->waitflags &= ~THREAD_WS_CPUIRECV_WAIT;
		pthread_unlock(&pthread->waitlock);
		pthread_sched_setrunnable(pthread);
		return 0;
	}
	
	return 0;
}

/*
 * This call is intended to make the given thread appear as if it is
 * in a scheduler message wait during a donation.
 */
int
pthread_sched_recv_wait(pthread_thread_t *pthread, schedmsg_t *msg)
{
	/*
	 * First, check to see if there is a message pending. Have the
	 * donator process that message first.
	 */
	pthread_lock(&pthread->msgqueue->lock);
	if (!schedmsg_queue_empty(pthread->msgqueue)) {
		schedmsg_dequeue(pthread->msgqueue, msg);
		pthread_unlock(&pthread->msgqueue->lock);
		return 1;
	}
	pthread_unlock(&pthread->msgqueue->lock);

	/*
	 * Nothing waiting ...
	 */
	pthread->waitflags |= THREAD_WS_CPUIRECV_WAIT;
	return 0;
}	

/*
 * Undo the effects of the previous function.  Return the pid of the
 * thread that sent it a message (if there was one).
 *
 * The thread should be locked.
 */
int
pthread_sched_recv_unwait(pthread_thread_t *pthread, schedmsg_t *msg)
{
	pthread->waitflags &= ~THREAD_WS_CPUIRECV_WAIT;
	
	pthread_lock(&pthread->msgqueue->lock);
	if (!schedmsg_queue_empty(pthread->msgqueue)) {
		schedmsg_dequeue(pthread->msgqueue, msg);
		pthread_unlock(&pthread->msgqueue->lock);
		return 1;
	}
	pthread_unlock(&pthread->msgqueue->lock);

	return 0;
}

/*
 * Cancelation support. Terminate a CPUI recv wait. 
 *
 * The thread is locked.
 */
void
pthread_sched_recv_cancel(pthread_thread_t *pthread)
{
	pthread->waitflags &= ~THREAD_WS_CPUIRECV_WAIT;
	pthread_unlock(&pthread->waitlock);
	pthread_sched_setrunnable(pthread);
}
#endif /* CPU_INHERIT */
