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
 * Set priority of a thread.
 */
#include <threads/pthread_internal.h>
#include "pthread_schedconf.h"

int
pthread_setschedparam(pthread_t tid, int policy,
		      const struct sched_param *param)
{
	pthread_thread_t  *pthread;
	int err;

	if ((pthread = tidtothread(tid)) == NULL_THREADPTR)
		return EINVAL;
	if ((err = pthread->scheduler->check_schedstate(param)) != 0)
		return err;

	assert_preemption_enabled();
	disable_preemption();

	/*
	 * Pass this off to the scheduler module. 
	 */
	pthread_sched_change_state(pthread, policy, param);

	enable_preemption();
	return 0;
}

