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

#include <threads/pthread_internal.h>

/*
 * Switch to another thread. This is the *only* external interface.
 */
void
sched_yield(void)
{
	assert_preemption_enabled();
	
	/*
	 * Let scheduler module decide what to do and actually do it.
	 */
	pthread_sched_reschedule(RESCHED_USERYIELD, (spin_lock_t *) 0);
}

/*
 * Internal interfaces. This is called whenever the runq is changed to
 * look for a rescheduling opportunity.
 */
void
pthread_yield(void)
{
	assert_preemption_enabled();
	
	/*
	 * Let scheduler module decide what to do and actually do it.
	 */
	pthread_sched_reschedule(RESCHED_YIELD, (spin_lock_t *) 0);
}

/*
 * This version is called from the preemption code so that a time preemption
 * can be distinguished from a voluntary yield.
 */
void
pthread_preempt(void)
{
	assert_preemption_enabled();
	assert_interrupts_disabled();

	/*
	 * Let scheduler module decide what to do ...
	 */
	pthread_sched_reschedule(RESCHED_PREEMPT, (spin_lock_t *) 0);
}
