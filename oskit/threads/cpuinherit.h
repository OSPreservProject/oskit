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

/*
 * External definitions for CPU inheritance.
 */
#ifndef	_CPUINHERIT_H_
#define _CPUINHERIT_H_

#include <oskit/error.h>

/*
 * CPU inheritance message types. 
 */
typedef enum {
	MSG_SCHED_NOTSETYET = 0,
	MSG_SCHED_UNBLOCK,		/* Thread is requesting reschedule */
	MSG_SCHED_SETSTATE,		/* Change thread state (priority) */
	MSG_SCHED_EXITED,		/* Thread is exiting */
	MSG_SCHED_NEWTHREAD,		/* Tell scheduler about new thread */
	MSG_SCHED_MAXTYPE,
} schedmsg_types_t;

extern char *msg_sched_typenames[];	/* Type strings for debugging */

/*
 * Return codes from thread donation. Note that the MESSAGE type is
 * combined with others. Caller must look for both a standard return
 * code, plus a message after a donating receive. This is not ideal
 * but its too inefficent to present everything as a message.
 */
#define SCHEDULE_NOTREADY	0x01
#define SCHEDULE_BLOCKED	0x02	/* Thread Blocked */
#define SCHEDULE_YIELDED	0x03	/* Thread called yield() */
#define SCHEDULE_PREEMPTED	0x04	/* Thread was preempted */
#define SCHEDULE_TIMEDOUT	0x05	/* Time based preemption */
#define SCHEDULE_MSGRECV	0x80	/* Message received as well */

extern char *msg_sched_rcodes[];

/*
 * Message format for scheduler messages. This is ad-hoc until we settle
 * on a message format.
 */
typedef struct schedmsg {
	schedmsg_types_t	type;	/* Message type code */
	pthread_t		tid;	/* Thread under operation */
	oskit_u32_t		opaque;	/* Extra data if appropriate */
	oskit_u32_t		opaque2;/* Another extra data word */
} schedmsg_t;

/*
 * Wakeup conditions for pthread_sched_donate().
 */
typedef enum {
	WAKEUP_NOTSETYET  = 0,
	WAKEUP_NEVER,			/* Internal. DO NOT USE! */
	WAKEUP_ON_BLOCK,
	WAKEUP_ON_SWITCH,
	WAKEUP_ALWAYS,
} sched_wakecond_t;

/*
 * Core cpu inheritance prototypes.
 */
/*
 * Donate CPU to a thread. Called by schedulers to donate CPU time to
 * the next thread it wants to run. Wakeup condition indicates under
 * what circumstance the thread donating should be woken up when the
 * target blocks. Timeout is in milliseconds; control returns if the
 * timeout expires, or if a message is sent to the scheduler's message
 * queue.
 *
 * The return value is the status of the donation. See the flags above.
 */
int		pthread_sched_donate_wait_recv(pthread_t tid,
			sched_wakecond_t cond, schedmsg_t *msg,
			oskit_s32_t timeout);

/*
 * Donating condition_wait operation. This operates the same as regular
 * condition_wait, except that the thread going into the wait can specify
 * that a target thread receive its CPU. An example usage would be to
 * specify join as a donating condition_wait so that the thread being waited
 * on will receive the waiter's CPU time. 
 */
int		pthread_cond_donate_wait(pthread_cond_t *c, pthread_mutex_t *m,
			oskit_timespec_t *abstime, pthread_t donee_tid);

/*
 * A scheduler starts up and tells the core that its a scheduler so that
 * it can perform the necessary initializations. The primary side effect
 * of this call is to create and attach a message queue to the thread so
 * that it can receive scheduler messages.
 */
void		pthread_sched_become_scheduler(void);

/*
 * Alter the scheduling parameters for a thread. Currently, just one
 * opaque value is supported (tickets, priority, reservation, etc).
 * This will need to beef up at some point.
 */
int		pthread_sched_setstate(pthread_t tid, int opaque);

/*
 * Message receive. Wait for a scheduler message to arrive. Timeout is
 * currently limited to 0 (do not block), and non-zero (block). Returns
 * zero status if a message is available. OSKIT_EAGAIN if a message is
 * not available and the call was non-blocking. 
 */
oskit_error_t	pthread_sched_message_recv(schedmsg_t *msg,
			oskit_s32_t timeout);

/*
 * Additional attribute support.
 */
/*
 * Set an opaque value that represents scheduling information.
 * ie: priority, tickets, reservation, etc.
 */
int		pthread_attr_setopaque(pthread_attr_t *attr, oskit_u32_t op);

/*
 * Set the parent scheduler for a newly created thread. By default, a
 * thread will be created with the same parent scheduler as the thread
 * that created it. Set it to a non-zero TID to reparent the new thread.
 */
int		pthread_attr_setscheduler(pthread_attr_t *attr, pthread_t tid);

/*
 * Currently defined schedulers. These routines create a new thread,
 * which acts as a scheduler thread. Scheduling messages can be sent
 * to named tid, although most messages originate withing the cpu
 * inheritance framework. For example, pthread_create() will send a 
 * MSG_SCHED_NEWTHREAD to the new threads scheduler (as defined in the
 * attribute structure).
 */

/*
 * Fixed Priority. Either FIFO or RR, as in POSIX.
 */
int		create_fixedpri_scheduler(pthread_t *tid,
			const pthread_attr_t *attr, int preemptible);

/*
 * Lottery
 */
int		create_lotto_scheduler(pthread_t *tid,
			const pthread_attr_t *attr, int preemptible);

/*
 * Stride
 */
int		create_stride_scheduler(pthread_t *tid,
			const pthread_attr_t *attr, int preemptible);

/*
 * Rate Monotonic
 */
int		create_ratemono_scheduler(pthread_t *tid,
			const pthread_attr_t *attr, int preemptible);

/*
 * Root scheduler prototype to get it bootstrapped. This function is
 * internal in that the pthread initialization code will call this
 * to get the root scheduler setup. However, it is possible for any
 * scheduler to provide this function, thus allowing the root
 * scheduler to be something other than the simple fixed priority version.
 */
void		bootstrap_root_scheduler(pthread_t tid, int preemptible,
			void *(**function)(void *), void **argument);

/*
 * Debug stuff.
 */
extern int			cpuinherit_debug;
int				cpuprintf(const char *__format, ...);

#define CPUDEBUG_TRUE		0x8000
#define CPUDEBUG_CORE0		0x0001
#define CPUDEBUG_CORE1		0x0002
#define CPUDEBUG_CORE2		0x0004
#define CPUDEBUG_CORE3		0x0008
#define CPUDEBUG_FIXEDPRI	0x0010
#define CPUDEBUG_RATEMONO	0x0020
#define CPUDEBUG_LOTTO		0x0040
#define CPUDEBUG_STRIDE		0x0080

#define CPUDEBUG(cond, args...) ({		\
	if (cpuinherit_debug & CPUDEBUG_##cond)	\
		cpuprintf(##args);		\
	})

#define CPUDEBUG_ISSET(cond)	(cpuinherit_debug & CPUDEBUG_##cond)

#endif /* _CPUINHERIT_H_ */
