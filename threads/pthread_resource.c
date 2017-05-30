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
 * Resource usage and stuff.
 */
#include <threads/pthread_internal.h>
#ifdef HIGHRES_THREADTIMES
#include "machine/hirestime.h"
#endif

/*
 * Return accumulated runtime of thread in machine-specific CPU cycles.
 */
int
oskit_pthread_cpucycles(pthread_t tid, long long *cycles)
{
#ifdef HIGHRES_THREADTIMES
	pthread_thread_t  *pthread;

	if ((pthread = tidtothread(tid)) == NULL_THREADPTR)
		return EINVAL;

	*cycles = pthread->cpucycles;
	return 0;
#else
	return EINVAL;
#endif
}

/*
 * Quite stupid, but all I need for right now. At some point I'll
 * set this up properly. This returns milliseconds.
 */
oskit_u32_t
oskit_pthread_cputime(pthread_t tid)
{
	pthread_thread_t  *pthread;

	if ((pthread = tidtothread(tid)) == NULL_THREADPTR)
		return EINVAL;

	return pthread->cputime * PTHREAD_TICK;
}

/*
 * Ditto! Return real time.
 */
oskit_u32_t
oskit_pthread_realtime(void)
{
	extern oskit_u32_t	threads_realticks;
	
	return threads_realticks * PTHREAD_TICK;
}

/*
 * Return CPU percentage as an integer value. Good enough!
 */
int
oskit_pthread_cpu_percent(void)
{
	return CURPTHREAD()->pctcpu;
}

#ifdef CPU_INHERIT
/*
 * Return the amount of time consumed by a thread and all its children.
 */
oskit_u32_t
oskit_pthread_childtime(pthread_t tid)
{
	pthread_thread_t  *pthread;

	if ((pthread = tidtothread(tid)) == NULL_THREADPTR)
		return EINVAL;

	return (pthread->cputime + pthread->childtime) * PTHREAD_TICK;
}
#endif
