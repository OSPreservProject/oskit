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
 * Scheduler definitions
 */
#ifndef	_OSKIT_THREADS_SCHED_H
#define _OSKIT_THREADS_SCHED_H

#include <oskit/time.h>

/*
 * Scheduler types. 
 */
enum {
	SCHED_FIFO      =	0x01,
	SCHED_RR        =	0x02,
	SCHED_DECAY     =	0x04,
	/*
	 * Realtime additions.
	 */
	SCHED_EDF	=	0x10,
	SCHED_RMS	=	0x20,

	/*
	 * More schedulers.
	 */
	SCHED_STRIDE    =	0x100,
};
#define SCHED_POLICY_POSIX(x)		((unsigned int) (x) & 0x0F)
#define SCHED_POLICY_REALTIME(x)	((unsigned int) (x) & 0xF0)

/*
 * Thread priorities.
 */
enum {
	PRIORITY_MIN	=	0,
	PRIORITY_NORMAL =	16,
	PRIORITY_MAX	=	31,
};

typedef struct sched_param {
	int			priority;
	/*
	 * Realtime additions. Works for EDF and RMS. Add as needed.
	 */
	oskit_timespec_t	start;
	oskit_timespec_t	deadline;
	oskit_timespec_t	period;
	/*
	 * Stride and maybe Lottery.
	 */
	int			tickets;
} sched_param_t;

#endif /* _OSKIT_THREADS_SCHED_H */


