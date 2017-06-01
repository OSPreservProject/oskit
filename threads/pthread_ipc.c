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

/*
 * Simple IPC.
 */
#include <threads/pthread_internal.h>
#include <threads/pthread_ipc.h>
#include <stdlib.h>
#include <string.h>

#define MIN(x, y)		((x) <= (y) ? (x) : (y))
#define ANYTID			((pthread_t) -1)
#define NOTID			((pthread_t) 0)

#ifdef	IPC_STATS
static struct ipc_stats stats;
void		dump_ipc_stats();
#endif

void
pthread_init_ipc(void)
{
#ifdef  IPC_STATS
	atexit(dump_ipc_stats);
#endif
}

/*
 * Send a message. Synchronous. Caller blocks until receiver picks
 * up the message, or until the timeout expires.
 */
oskit_error_t
oskit_ipc_send(pthread_t dst,
	       void *msg, oskit_size_t msg_size, oskit_s32_t timeout)
{
	pthread_thread_t	*pthread = CURPTHREAD();
	pthread_thread_t	*target;
	int			err = 0;
#ifdef  IPC_STATS
	stat_stamp_t		before;

	before = STAT_STAMPGET();
#endif

	pthread_testcancel();

	if ((target = tidtothread(dst)) == NULL_THREADPTR)
		return EINVAL;

	/*
	 * To avoid deadlock, everyone locks the recv side first.
	 */
	assert_interrupts_enabled();
	disable_interrupts();
	pthread_lock(&target->waitlock);

	/*
	 * Check the target thread. If its already waiting for a message
	 * from *this* thread, go ahead and deliver the message.
	 */
	if (target->waitflags & THREAD_WS_IPCRECV_WAIT &&
	    (target->ipc_state.tid == pthread->tid ||
	     target->ipc_state.tid == ANYTID)) {
		if (msg_size) {
			memcpy(target->ipc_state.msg,
			       msg, MIN(msg_size, target->ipc_state.msg_size));

			if (msg_size > target->ipc_state.msg_size)
				err = OSKIT_ERANGE;
		}

		/*
		 * Tell the receiver the amount of data sent, and who sent
		 * it in case the receiver was in a wait instead of a recv.
		 */
		target->ipc_state.msg_size = msg_size;
		target->ipc_state.tid      = pthread->tid;

		/*
		 * Clear the recv wait.
		 */
		target->waitflags &= ~THREAD_WS_IPCRECV_WAIT;
		pthread_unlock(&target->waitlock);
#ifdef  IPC_STATS
		stats.sends++;
		stats.send_cycles += STAT_STAMPDIFF(before);
#endif
		/*
		 * And handoff to the receiver. No waitstate is provided,
		 * so the sender yields to the receiver (sender does not
		 * block).
		 */
		pthread_sched_handoff(0, target);
		enable_interrupts();
		return err;
	}

	pthread_lock(&pthread->waitlock);

	/*
	 * Add to receiver's senders queue. Necessary if the receiver does
	 * a wait instead of a recv.
	 */
	queue_enter(&target->ipc_state.senders,
		    pthread, pthread_thread_t *, ipc_state.senders_chain)

	pthread->ipc_state.msg      = msg;
	pthread->ipc_state.msg_size = msg_size;
	pthread->ipc_state.tid      = dst;
	pthread->waitflags         |= THREAD_WS_IPCSEND_WAIT;
	pthread_unlock(&target->waitlock);
#ifdef  IPC_STATS
	stats.sblocks++;
	stats.sblock_cycles += STAT_STAMPDIFF(before);
#endif
	pthread_sched_reschedule(RESCHED_BLOCK, &pthread->waitlock);

	/*
	 * Back from send. The receiver woke us up, or we got canceled.
	 */
	pthread_lock(&pthread->lock);
	if (pthread->flags & THREAD_CANCELED) {
		pthread_unlock(&pthread->lock);
		enable_interrupts();
		return OSKIT_ECANCELED;
	}
	pthread_unlock(&pthread->lock);

	/*
	 * Message was transfered. Check for short message.
	 */
	if (msg_size > pthread->ipc_state.msg_size)
		err = OSKIT_ERANGE;

	enable_interrupts();
	return err;
}

/*
 * Blocking receive. The receiver waits for a message from the specified
 * source thread.
 */
oskit_error_t
oskit_ipc_recv(pthread_t src,
	       void *msg, oskit_size_t msg_size, oskit_size_t *actual,
	       oskit_s32_t timeout)
{
	pthread_thread_t	*pthread = CURPTHREAD();
	pthread_thread_t	*source;
	int			err = 0;
#ifdef  IPC_STATS
	stat_time_t		before;

	before = STAT_STAMPGET();
#endif

	pthread_testcancel();

	if ((source = tidtothread(src)) == NULL_THREADPTR)
		return EINVAL;

	/*
	 * To avoid deadlock, everyone locks the recv side first.
	 */
	assert_interrupts_enabled();
	disable_interrupts();
	pthread_lock(&pthread->waitlock);

	/*
	 * Look to see if the source thread is in the send side.
	 * If so, the message can be transfered from the sender
	 * and the sender woken up.
	 */
	pthread_lock(&source->waitlock);
	if (source->waitflags & THREAD_WS_IPCSEND_WAIT &&
	    source->ipc_state.tid == pthread->tid) {
		*actual = MIN(msg_size, source->ipc_state.msg_size);

		if (*actual) {
			memcpy(msg, source->ipc_state.msg, *actual);

			if (source->ipc_state.msg_size > msg_size)
				err = OSKIT_ERANGE;
		}

		/*
		 * Since the sender was waiting, it was on the receiver's
		 * senders queue. Remove it.
		 */
		queue_remove(&pthread->ipc_state.senders, source,
			     pthread_thread_t *, ipc_state.senders_chain);

		/*
		 * Tell the sender the amount of data received.
		 */
		source->ipc_state.msg_size = *actual;

		/*
		 * Clear the send wait. If the the sender side initiated
		 * an RPC, do not wake it up yet. The reply will do it.
		 */
		source->waitflags &= ~THREAD_WS_IPCSEND_WAIT;
		if (! (source->waitflags & THREAD_WS_IPCREPL_WAIT)) {
			pthread_unlock(&source->waitlock);
			pthread_sched_setrunnable(source);
		}
		else
			pthread_unlock(&source->waitlock);

		pthread_unlock(&pthread->waitlock);
#ifdef  IPC_STATS
		stats.recvs++;
		stats.recv_cycles += STAT_STAMPDIFF(before);
#endif
		enable_interrupts();
		return err;
	}
	pthread_unlock(&source->waitlock);

	/*
	 * No sender is waiting. Set up the receiver to wait for
	 * a sender.
	 */
	pthread->ipc_state.msg      = msg;
	pthread->ipc_state.msg_size = msg_size;
	pthread->ipc_state.tid      = src;
	pthread->waitflags         |= THREAD_WS_IPCRECV_WAIT;
#ifdef  IPC_STATS
	stats.rblocks++;
	stats.rblock_cycles += STAT_STAMPDIFF(before);
#endif
	pthread_sched_reschedule(RESCHED_BLOCK, &pthread->waitlock);

	/*
	 * Back from receiver. The sender woke us up, or we got canceled.
	 */
	pthread_lock(&pthread->lock);
	if (pthread->flags & THREAD_CANCELED) {
		pthread_unlock(&pthread->lock);
		enable_interrupts();
		return OSKIT_ECANCELED;
	}
	pthread_unlock(&pthread->lock);

	/*
	 * Message was transfered. Check for short message.
	 */
	*actual = pthread->ipc_state.msg_size;
	if (pthread->ipc_state.msg_size > msg_size)
		err = OSKIT_ERANGE;

	enable_interrupts();
	return err;
}

/*
 * Blocking wait. The receiver waits for a message from any source thread.
 * The id of the source is returned.
 */
oskit_error_t
oskit_ipc_wait(pthread_t *src,
	       void *msg, oskit_size_t msg_size, oskit_size_t *actual,
	       oskit_s32_t timeout)
{
	pthread_thread_t	*pthread = CURPTHREAD();
	pthread_thread_t	*source;
	int			err = 0;
#ifdef  IPC_STATS
	stat_stamp_t		before;

	before = STAT_STAMPGET();
#endif
	pthread_testcancel();

	/*
	 * To avoid deadlock, everyone locks the recv side first.
	 */
	assert_interrupts_enabled();
	disable_interrupts();
	pthread_lock(&pthread->waitlock);

	/*
	 * Since this is non-specific receive, it is up to the senders
	 * to initiate. Look for a waiting sender in the senders queue
	 * first.
	 */
	if (queue_empty(&pthread->ipc_state.senders))
		goto wait;

	/*
	 * A sender is waiting.
	 */
	queue_remove_first(&pthread->ipc_state.senders, source,
			   pthread_thread_t *, ipc_state.senders_chain);

	pthread_lock(&source->waitlock);

	assert(source->waitflags & THREAD_WS_IPCSEND_WAIT &&
	       source->ipc_state.tid == pthread->tid);

	/*
	 * The message can be transfered from the sender, and then the
	 * sender is woken up.
	 */
	*actual = MIN(msg_size, source->ipc_state.msg_size);

	if (*actual) {
		memcpy(msg, source->ipc_state.msg, *actual);

		if (source->ipc_state.msg_size > msg_size)
			err = OSKIT_ERANGE;
	}

	/*
	 * Tell the sender the amount of data received.
	 */
	source->ipc_state.msg_size = *actual;

	/*
	 * Tell the caller who sent the message.
	 */
	*src = source->tid;

	/*
	 * Clear the send wait. If the the sender side initiated
	 * an RPC, do not wake it up yet. The reply will do it.
	 */
	source->waitflags &= ~THREAD_WS_IPCSEND_WAIT;
	if (! (source->waitflags & THREAD_WS_IPCREPL_WAIT)) {
		pthread_unlock(&source->waitlock);
		pthread_sched_setrunnable(source);
	}
	else
		pthread_unlock(&source->waitlock);

	pthread_unlock(&pthread->waitlock);
#ifdef  IPC_STATS
	stats.waits++;
	stats.wait_cycles += STAT_STAMPDIFF(before);
#endif
	enable_interrupts();
	return err;

   wait:
	/*
	 * Only two timeout values right now, 0 and forever ...
	 */
	if (timeout == 0) {
		pthread_unlock(&pthread->waitlock);
#ifdef  IPC_STATS
		stats.wfails++;
		stats.wfail_cycles += STAT_STAMPDIFF(before);
#endif
		enable_interrupts();
		return OSKIT_EAGAIN;
	}

	/*
	 * The receiver must wait ...
	 */
	pthread->ipc_state.msg      = msg;
	pthread->ipc_state.msg_size = msg_size;
	pthread->ipc_state.tid      = ANYTID;
	pthread->waitflags         |= THREAD_WS_IPCRECV_WAIT;
#ifdef  IPC_STATS
	stats.wblocks++;
	stats.wblock_cycles += STAT_STAMPDIFF(before);
#endif
	pthread_sched_reschedule(RESCHED_BLOCK, &pthread->waitlock);

	/*
	 * Back from receiver. The sender woke us up, or we got canceled.
	 */
	pthread_lock(&pthread->lock);
	if (pthread->flags & THREAD_CANCELED) {
		pthread_unlock(&pthread->lock);
		enable_interrupts();
		return OSKIT_ECANCELED;
	}
	pthread_unlock(&pthread->lock);

	/*
	 * Message was transfered. Check for short message.
	 */
	*src    = pthread->ipc_state.tid;
	*actual = pthread->ipc_state.msg_size;
	if (pthread->ipc_state.msg_size > msg_size)
		err = OSKIT_ERANGE;

	enable_interrupts();
	return err;
}

/*
 * Send a message. Synchronous. Caller blocks until receiver picks
 * up the message, or until the timeout expires.
 */
oskit_error_t
oskit_ipc_call(pthread_t dst,
	       void *sendmsg, oskit_size_t sendmsg_size,
	       void *recvmsg, oskit_size_t recvmsg_size, oskit_size_t *actual,
	       oskit_s32_t timeout)
{
	pthread_thread_t	*pthread = CURPTHREAD();
	pthread_thread_t	*target;
	int			err = 0;

	pthread_testcancel();

	if ((target = tidtothread(dst)) == NULL_THREADPTR)
		return EINVAL;

	/*
	 * To avoid deadlock, everyone locks the recv side first.
	 */
	assert_interrupts_enabled();
	disable_interrupts();
	pthread_lock(&target->waitlock);

	/*
	 * Check the target thread. If its already waiting for a message
	 * from *this* thread, go ahead and deliver the message.
	 */
	if (target->waitflags & THREAD_WS_IPCRECV_WAIT &&
	    (target->ipc_state.tid == pthread->tid ||
	     target->ipc_state.tid == ANYTID)) {
		if (sendmsg_size) {
			memcpy(target->ipc_state.msg, sendmsg,
			       MIN(sendmsg_size, target->ipc_state.msg_size));

			if (sendmsg_size > target->ipc_state.msg_size)
				err = OSKIT_ERANGE;
		}

		/*
		 * Tell the receiver the amount of data sent, and who sent
		 * it in case the receiver was in a wait instead of a recv.
		 */
		target->ipc_state.msg_size = sendmsg_size;
		target->ipc_state.tid      = pthread->tid;

		/*
		 * Setup sender to recv the reply.
		 */
		pthread->ipc_state.reply      = recvmsg;
		pthread->ipc_state.reply_size = recvmsg_size;
		pthread->ipc_state.tid        = dst;

		/*
		 * Clear the recv wait.
		 */
		target->waitflags &= ~THREAD_WS_IPCRECV_WAIT;
		pthread_unlock(&target->waitlock);

		/*
		 * And handoff to the receiver, waiting for reply message.
		 */
		pthread_sched_handoff(THREAD_WS_IPCREPL_WAIT, target);
		goto reply;
	}

	pthread_lock(&pthread->waitlock);

	/*
	 * Add to receiver's senders queue. Necessary if the receiver does
	 * a wait instead of a recv.
	 */
	queue_enter(&target->ipc_state.senders,
		    pthread, pthread_thread_t *, ipc_state.senders_chain);

	pthread->ipc_state.msg        = sendmsg;
	pthread->ipc_state.msg_size   = sendmsg_size;
	pthread->ipc_state.reply      = recvmsg;
	pthread->ipc_state.reply_size = recvmsg_size;
	pthread->ipc_state.tid        = dst;
	pthread->waitflags           |= (THREAD_WS_IPCSEND_WAIT|
					 THREAD_WS_IPCREPL_WAIT);
	pthread_unlock(&target->waitlock);
	pthread_sched_reschedule(RESCHED_BLOCK, &pthread->waitlock);

	/*
	 * Back from call. The receiver woke us up with the reply, or we
	 * got canceled.
	 */
  reply:
	pthread_lock(&pthread->lock);
	if (pthread->flags & THREAD_CANCELED) {
		pthread_unlock(&pthread->lock);
		enable_interrupts();
		return OSKIT_ECANCELED;
	}
	pthread_unlock(&pthread->lock);

	/*
	 * Message was transfered. Check for short message.
	 */
	*actual = pthread->ipc_state.reply_size;
	if (pthread->ipc_state.reply_size > recvmsg_size)
		err = OSKIT_ERANGE;

	enable_interrupts();
	return err;
}

/*
 * Reply to an RPC. The target thread must be in the RPC client side
 * wait, or the call fails.
 */
oskit_error_t
oskit_ipc_reply(pthread_t src, void *msg, oskit_size_t msg_size)
{
	pthread_thread_t	*pthread = CURPTHREAD();
	pthread_thread_t	*source;
	int			err = 0;

	if ((source = tidtothread(src)) == NULL_THREADPTR)
		return OSKIT_EINVAL;

	/*
	 * To avoid deadlock, everyone locks the recv side first.
	 */
	assert_interrupts_enabled();
	disable_interrupts();
	pthread_lock(&pthread->waitlock);

	/*
	 * Check the sender to make sure its in a RPC reply wait from this
	 * thread. Error if not.
	 */
	pthread_lock(&source->waitlock);
	if (! (source->waitflags & THREAD_WS_IPCREPL_WAIT &&
	       source->ipc_state.tid == pthread->tid)) {

		pthread_unlock(&source->waitlock);
		pthread_unlock(&pthread->waitlock);
		enable_interrupts();
		return OSKIT_EINVAL;
	}

	/*
	 * Copy back the reply and look for a short message.
	 */
	if (msg_size) {
		memcpy(source->ipc_state.reply,
		       msg, MIN(msg_size, source->ipc_state.reply_size));

		if (msg_size > source->ipc_state.reply_size)
			err = OSKIT_ERANGE;
	}

	/*
	 * Tell the sender the size of the reply.
	 */
	source->ipc_state.reply_size = msg_size;

	/*
	 * Clear the reply wait.
	 */
	source->waitflags &= ~THREAD_WS_IPCREPL_WAIT;
	pthread_unlock(&source->waitlock);
	pthread_unlock(&pthread->waitlock);

	/*
	 * And handoff back to the sender. No waitstate is provided,
	 * so we yield to the sender (we do not block).
	 */
	pthread_sched_handoff(0, source);
	enable_interrupts();
	return err;
}

/*
 * Cancelation Support.
 */
void
pthread_ipc_cancel(pthread_thread_t *pthread)
{
	if (pthread->waitflags & THREAD_WS_IPCRECV_WAIT) {
		pthread->waitflags &= ~THREAD_WS_IPCRECV_WAIT;
		pthread_unlock(&pthread->waitlock);
		pthread_sched_setrunnable(pthread);
	}
	else
		panic("oskit_ipc_cancel");
}

#ifdef IPC_STATS
void
dump_ipc_stats()
{
	printf("IPC Stats:\n");
	printf("Sends:          %d\n", stats.sends);
	printf("Send Cycles:    %lu\n", stats.send_cycles);
	printf("Send Blocks:    %d\n", stats.sblocks);
	printf("Send Block Cyc: %lu\n", stats.sblock_cycles);
	printf("Recvs:          %d\n", stats.recvs);
	printf("Recv Cycles:    %lu\n", stats.recv_cycles);
	printf("Recv Blocks:    %d\n", stats.rblocks);
	printf("Recv Block Cyc: %lu\n", stats.rblock_cycles);
	printf("Waits:          %d\n", stats.waits);
	printf("Wait Cycles:    %lu\n", stats.wait_cycles);
	printf("Wait Fails:     %d\n", stats.wfails);
	printf("Wait Fail Cyc:  %lu\n", stats.wfail_cycles);
	printf("Wait Blocks:    %d\n", stats.wblocks);
	printf("Wait Block Cyc: %lu\n", stats.wblock_cycles);
	printf("Sched Msgs:     %d\n", stats.scheds);
	printf("Sched Msg Cyc:  %lu\n", stats.sched_cycles);
	printf("Sched NotR:     %d\n", stats.schednrs);
	printf("Sched NotR Cyc: %lu\n", stats.schednr_cycles);
	printf("Sched Waits:    %d\n", stats.schedws);
	printf("Sched Wait Cyc: %lu\n", stats.schedw_cycles);
}
#endif
