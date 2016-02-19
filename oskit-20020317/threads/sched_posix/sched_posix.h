/*
 * Copyright (c) 2000 University of Utah and the Flux Group.
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
 * Default POSIX SCHED_FIFO and SCHED_RR policies ...
 */
#ifndef  WEAK_WORKING
#define PWEAK
#else
#define PWEAK	__attribute__ ((weak))
#endif

void
posix_sched_init(void)					PWEAK;

int
posix_sched_schedules(int policy)			PWEAK;

int
posix_sched_dispatch(resched_flags_t reason,
		pthread_thread_t *pthread)		PWEAK;

pthread_thread_t *
posix_sched_thread_next(void)				PWEAK;

int
posix_sched_change_state(pthread_thread_t *pthread,
		const struct sched_param *param)	PWEAK;

int
posix_sched_setrunnable(pthread_thread_t *pthread)	PWEAK;

void
posix_sched_disassociate(pthread_thread_t *pthread)	PWEAK;

int
posix_sched_check_schedstate(
		const struct sched_param *param)	PWEAK;

void
posix_sched_init_schedstate(pthread_thread_t *pthread,
		const struct sched_param *param)	PWEAK;

int
posix_sched_priority_bump(pthread_thread_t *pthread,
			  int newprio)			PWEAK;

#ifdef PRI_INHERIT
void
pthread_priority_inherit(pthread_thread_t *pthread)	PWEAK;

void
pthread_priority_uninherit(pthread_thread_t *pthread)	PWEAK;

void
pthread_priority_kill(pthread_thread_t *pthread)	PWEAK;

void
pthread_priority_decreasing_recompute(pthread_thread_t *pthread);

void
pthread_priority_increasing_recompute(pthread_thread_t *pthread);
#endif
