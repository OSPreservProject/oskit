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

/*
 * Define the scheduler array entry for default scheduler. Each thread
 * contains a pointer to one of these, and is used to dispatch to the
 * current scheduler for that thread.
 */
struct scheduler_entry {
	/*
	 * Initialize the scheduler.
	 */
	void		(*init)(void);

	/*
	 * Match policy to scheduler. Kinda hokey. Returns true/false.
	 */
	int		(*schedules)(int policy);
	
	/*
	 * Dispatch a thread back to its scheduler.
	 */
	int		(*dispatch)(resched_flags_t reason,
				pthread_thread_t *pthread);

	/*
	 * Select next thread to run, if any.
	 */
	pthread_thread_t *(*thread_next)(void);

	/*
	 * Change state (priority, policy, etc).
	 */
	int		(*change_state)(pthread_thread_t *pthread,
				const struct sched_param *param);

	/*
	 * Set thread runnable.
	 */
	int		(*setrunnable)(pthread_thread_t *pthread);

	/*
	 * Terminate scheduler/thread association (as would happen
	 * if you did something silly like change the policy from
	 * RR to EDF).
	 */
	void		(*disassociate)(pthread_thread_t *pthread);

	/*
	 * Validate proposed scheduler state
	 * Returns 0 if ok, an error otherwise.
	 */
	int		(*check_schedstate)(const struct sched_param *param);

	/*
	 * Init scheduler state for a new thread.
	 */
	void		(*init_schedstate)(pthread_thread_t *pthread,
				const struct sched_param *param);

	/*
	 * Bump priority for priority transfer. Used for simple priority
	 * inheritance.
	 */
	int		(*priority_bump)(pthread_thread_t *pthread,
				int newprio);
};

extern struct scheduler_entry	scheduler_array[];
extern int    nschedulers;
