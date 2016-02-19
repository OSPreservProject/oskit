/*
 * Copyright (c) 1996, 1998-2000 University of Utah and the Flux Group.
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

#ifdef	DEFAULT_SCHEDULER

/*
 * Default scheduler.
 */
#include <threads/pthread_internal.h>
#include "pthread_schedconf.h"

#ifdef PTHREAD_SCHED_POSIX
#include "sched_posix.h"
#endif
#ifdef PTHREAD_SCHED_STRIDE
#include "sched_stride.h"
#endif
#include "sched_realtime.h"

/*
 * List these according to priority. That is, realtime threads are higher
 * priority than POSIX (RR/FIFO) threads, so a realtime thread should be
 * chosen to run before a POSIX thread.
 *
 * The astute reader will realize that an arbitrary combination of schedulers
 * without hierarchy makes no sense. But, this is more pleasing to read than
 * a zillion ifdefs. In general, it makes (some amount of) sense to use the
 * POSIX scheduler and/or a realtime scheduler. Two different realtime 
 * schedulers, either with or without the POSIX scheduler seems like a bad
 * idea.
 *
 * It probably makes no sense to use the STRIDE scheduler with another
 * scheduler.
 */
struct scheduler_entry scheduler_array[] = {
#ifdef PTHREAD_SCHED_EDF
	{
		edf_sched_init,
		edf_sched_schedules,
		edf_sched_dispatch,
		edf_sched_thread_next,
		edf_sched_change_state,
		edf_sched_setrunnable,
		edf_sched_disassociate,
		edf_sched_check_schedstate,
		edf_sched_init_schedstate,
		edf_sched_priority_bump,
	},
#endif
#ifdef PTHREAD_SCHED_STRIDE
	{
		stride_sched_init,
		stride_sched_schedules,
		stride_sched_dispatch,
		stride_sched_thread_next,
		stride_sched_change_state,
		stride_sched_setrunnable,
		stride_sched_disassociate,
		stride_sched_check_schedstate,
		stride_sched_init_schedstate,
		stride_sched_priority_bump,
	},
#endif
#ifdef PTHREAD_SCHED_POSIX
	{
		posix_sched_init,
		posix_sched_schedules,
		posix_sched_dispatch,
		posix_sched_thread_next,
		posix_sched_change_state,
		posix_sched_setrunnable,
		posix_sched_disassociate,
		posix_sched_check_schedstate,
		posix_sched_init_schedstate,
		posix_sched_priority_bump,
	},
#endif
};
int	nschedulers = sizeof(scheduler_array) / sizeof(scheduler_array[0]);
#endif /* DEFAULT_SCHEDULER */
