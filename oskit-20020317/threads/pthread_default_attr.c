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
 * Basic attributes stuff.
 */
#include <threads/pthread_internal.h>
#include <oskit/c/string.h>

/*
 * Default attributes.
 */
pthread_attr_t		pthread_attr_default;
pthread_mutexattr_t	pthread_mutexattr_default;
pthread_condattr_t	pthread_condattr_default;

void
pthread_init_attributes(void)
{
	pthread_attr_default.detachstate = PTHREAD_CREATE_JOINABLE;
	pthread_attr_default.stackaddr   = 0;
	pthread_attr_default.stacksize   = PTHREAD_STACK_MIN;
	pthread_attr_default.guardsize   = DEFAULT_STACKGUARD;
	pthread_attr_default.policy      = SCHED_RR;
	pthread_attr_default.inherit     = PTHREAD_EXPLICIT_SCHED;
#ifdef	CPU_INHERIT
	pthread_attr_default.scheduler   = 0;
	pthread_attr_default.opaque      = 0;
#endif
	/*
	 * Gotta choose the default scheduling policy better then this!
	 */
#ifdef  PTHREAD_SCHED_POSIX
	pthread_attr_default.policy      = SCHED_RR;
#elif   defined(PTHREAD_SCHED_EDF)
	pthread_attr_default.policy      = SCHED_EDF;
#elif   defined(PTHREAD_SCHED_STRIDE)
	pthread_attr_default.policy      = SCHED_STRIDE;
#else
#error	Scheduler defintion problem!
#endif

#ifdef  PTHREAD_SCHED_POSIX
	pthread_attr_default.priority    = PRIORITY_NORMAL;
#endif
#ifdef  PTHREAD_REALTIME
	memset(&pthread_attr_default.start,    0,
	       sizeof(pthread_attr_default.start));
	memset(&pthread_attr_default.period,    0,
	       sizeof(pthread_attr_default.period));
	memset(&pthread_attr_default.deadline,    0,
	       sizeof(pthread_attr_default.deadline));
#endif
#ifdef PTHREAD_SCHED_STRIDE
	pthread_attr_default.tickets    = 100;
#endif
	pthread_mutexattr_default.priority_inherit = PTHREAD_PRIO_NONE;
	pthread_mutexattr_default.type             = PTHREAD_MUTEX_DEFAULT;
}

