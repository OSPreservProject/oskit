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
 * Internal definitions for CPU inheritance.
 */
#ifndef	_PTHREAD_CPUINHERIT_H_
#define _PTHREAD_CPUINHERIT_H_

#include <oskit/threads/cpuinherit.h>

/*
 * Scheduler flags specific to CPU inheritance. See pthread_sched_reschedule.
 */
typedef enum {
	SCHED_NOTSETYET    = 0,
	SCHED_DONATING,		/* Dontaing CPU to another thread */
	SCHED_RUNNING,		/* Thread has the CPU */
	SCHED_READY,		/* Thread is ready for the CPU */
	SCHED_WAITING,		/* Thread is waiting for something */
} schedflags_t;

/*
 * A message queue. This is a statically sized circular queue.
 */
typedef struct schedmsg_queue {
        pthread_lock_t  lock;
        oskit_u32_t     head;
        oskit_u32_t     tail;
        oskit_u32_t     mask;
        int             size;
        schedmsg_t      entries[1];
} schedmsg_queue_t;

/*
 * There is a single root scheduler.
 */
extern struct pthread_thread	*pthread_root_scheduler;

/*
 * Prototypes for main scheduler functions. These are internal to the thread
 * system. The external calls are defined in oskit/threads/cpuinherit.h
 */
void		pthread_sched_wakeup(struct pthread_thread *pthread, int lev);
void		pthread_sched_exit(void);
void		pthread_sched_switchto(struct pthread_thread *pthread);
void		pthread_sched_thread_wait(struct pthread_thread *waiting_on,
			queue_head_t *queue, pthread_lock_t *plock);
void		pthread_sched_clocktick(void);
int		pthread_sched_thread_donate(struct pthread_thread *pchild,
			sched_wakecond_t wakeup_cond, oskit_s32_t timeout);

/*
 * Message prototypes.
 */
schedmsg_queue_t	*schedmsg_queue_allocate(int size);
void			schedmsg_queue_deallocate(schedmsg_queue_t *queue);
oskit_error_t		pthread_sched_special_send(struct pthread_thread *p,
				schedmsg_t *msg);
int			pthread_sched_recv_wait(struct pthread_thread *p,
				schedmsg_t *msg);
int			pthread_sched_recv_unwait(struct pthread_thread *p,
				schedmsg_t *msg);
oskit_error_t		pthread_sched_message_send(struct pthread_thread *p,
				schedmsg_t *msg);
void			pthread_sched_recv_cancel(struct pthread_thread *p);

extern char *msg_sched_typenames[];	/* Type strings for debugging */
#endif /* _PTHREAD_CPUINHERIT_H_ */


