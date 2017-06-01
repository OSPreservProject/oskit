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
 * Local addition; retrieve the state of a target thread.       
 *
 * Returns ENIVAL if the thread is bogus, 0 on success.
 */
int
oskit_pthread_getstate(pthread_t tid, pthread_state_t *pstate)
{
	pthread_thread_t  *pthread;

	if ((pthread = tidtothread(tid)) == NULL_THREADPTR)
		return EINVAL;

	/*
	 * Kinda Bogus. There is no way to prevent a thread from being
	 * scheduled after the getstate is done, thus changing the state.
	 * It is up to the caller to prevent the target thread from running.
	 */
	disable_preemption();
	pthread_lock(&pthread->lock);

	/* Machine dependent routine. */
	thread_getstate(pthread, pstate);
	
	pthread_unlock(&pthread->lock);
	enable_preemption();
	return 0;
}
