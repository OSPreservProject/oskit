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

int
pthread_attr_init(pthread_attr_t *attr)
{
	attr->detachstate = PTHREAD_CREATE_JOINABLE;
	attr->stacksize   = PTHREAD_STACK_MIN;
	attr->guardsize   = DEFAULT_STACKGUARD;
	attr->stackaddr   = 0;
	attr->inherit     = PTHREAD_EXPLICIT_SCHED;
#ifdef	CPU_INHERIT
	attr->scheduler   = 0;
	attr->opaque      = 0;
#endif

	/*
	 * Gotta choose the default scheduling policy better then this!
	 */
#ifdef  PTHREAD_SCHED_POSIX
	attr->policy      = SCHED_RR;
#elif   defined(PTHREAD_SCHED_EDF)
	attr->policy      = SCHED_EDF;
#elif   defined(PTHREAD_SCHED_STRIDE)
	attr->policy      = SCHED_STRIDE;
#else
#error	Scheduler defintion problem!
#endif
	
#ifdef  PTHREAD_SCHED_POSIX
	attr->priority    = PRIORITY_NORMAL;
#endif	
#ifdef  PTHREAD_REALTIME
	memset(&attr->start,    0, sizeof(attr->start));
	memset(&attr->period,   0, sizeof(attr->period));
	memset(&attr->deadline, 0, sizeof(attr->deadline));
#endif
#ifdef  PTHREAD_SCHED_STRIDE
	attr->tickets     = 100;
#endif
	return 0;
}

int
pthread_attr_destroy(pthread_attr_t *attr)
{
	memset((void *) attr, -1, sizeof(*attr));

	return 0;
}

int
pthread_attr_setguardsize(pthread_attr_t *attr, size_t guardsize)
{
#ifdef	STACKGUARD
	if (guardsize < 0)
		return EINVAL;
	
	attr->guardsize = guardsize;
	return 0;
#else
	printf("pthread_attr_setguardsize: STACKGUARD not defined\n");
	return EINVAL;
#endif
}

int
pthread_attr_getguardsize(const pthread_attr_t *attr, size_t *guardsize)
{
#ifdef	STACKGUARD
	*guardsize = attr->guardsize;
	return 0;
#else
	printf("pthread_attr_getguardsize: STACKGUARD not defined\n");
	return EINVAL;
#endif
}

int
pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate)
{
	if (detachstate != PTHREAD_CREATE_JOINABLE &&
	    detachstate != PTHREAD_CREATE_DETACHED)
		return EINVAL;
	
	attr->detachstate = detachstate;

	return 0;
}

int
pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate)
{
	*detachstate = attr->detachstate;

	return 0;
}

int
pthread_attr_setinheritsched(pthread_attr_t *attr, int inheritstate)
{
	if (inheritstate != PTHREAD_INHERIT_SCHED &&
	    inheritstate != PTHREAD_EXPLICIT_SCHED)
		return EINVAL;
	
	attr->inherit = inheritstate;

	return 0;
}

int
pthread_attr_getinheritsched(const pthread_attr_t *attr, int *inheritstate)
{
	*inheritstate = attr->inherit;

	return 0;
}

int
pthread_attr_setschedparam(pthread_attr_t *attr,
			   const struct sched_param *param)
{
	int	pri = param->priority;
		
	if (pri < PRIORITY_MIN || pri > PRIORITY_MAX)
		return EINVAL;
#ifdef LATENCY_THREAD
	if (pri == PRIORITY_MAX)
		pri--;
#endif
	
	attr->priority    = pri;
#ifdef  PTHREAD_REALTIME
	attr->start       = param->start;
	attr->period      = param->period;
	attr->deadline    = param->deadline;
#endif
#ifdef  PTHREAD_SCHED_STRIDE
	attr->tickets     = param->tickets;
#endif
	return 0;
}

int
pthread_attr_getschedparam(pthread_attr_t *attr, struct sched_param *param)
{
	param->priority = attr->priority;
#ifdef  PTHREAD_REALTIME
	param->start    = attr->start;
	param->period   = attr->period;
	param->deadline = attr->deadline;
#endif
#ifdef  PTHREAD_SCHED_STRIDE
	param->tickets  = attr->tickets;
#endif
	return 0;
}

int
pthread_attr_setschedpolicy(pthread_attr_t *attr, int policy)
{
	switch (policy) {
	case SCHED_FIFO:
	case SCHED_RR:
#ifdef  PTHREAD_SCHED_EDF
	case SCHED_EDF:
#endif
#ifdef  PTHREAD_SCHED_RMS
	case SCHED_RMS:
#endif
#ifdef  PTHREAD_SCHED_STRIDE
	case SCHED_STRIDE:
#endif
		attr->policy = policy;
		break;

	case SCHED_DECAY:
		printf("pthread_attr_setschedpolicy: "
		       "SCHED_DECAY not supported yet\n");

	default:
		return EINVAL;
	}

	return 0;
}

int
pthread_attr_getschedpolicy(const pthread_attr_t *attr, int *policy)
{
	*policy = attr->policy;

	return 0;
}

int
pthread_attr_setstackaddr(pthread_attr_t *attr, void *stackaddr)
{
	attr->stackaddr = stackaddr;
	return 0;
}

int
pthread_attr_getstackaddr(const pthread_attr_t *attr, void **stackaddr)
{
	*stackaddr = attr->stackaddr;
	return 0;
}

int
pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize)
{
	if (stacksize < PTHREAD_STACK_MIN)
		return EINVAL;
	
	attr->stacksize = stacksize;

	return 0;
}

int
pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize)
{
	*stacksize = attr->stacksize;

	return 0;
}

/*
 * Local
 */
int
oskit_pthread_attr_setprio(pthread_attr_t *attr, int pri)
{
	if (pri < PRIORITY_MIN || pri > PRIORITY_MAX)
		return EINVAL;
#ifdef LATENCY_THREAD
	if (pri == PRIORITY_MAX)
		pri--;
#endif
	
	attr->priority = pri;
	
	return 0;
}

#ifdef	CPU_INHERIT
int
pthread_attr_setopaque(pthread_attr_t *attr, oskit_u32_t opaque)
{
	attr->opaque = opaque;
	
	return 0;
}

int
pthread_attr_setscheduler(pthread_attr_t *attr, pthread_t tid)
{
	if (tidtothread(tid) == NULL_THREADPTR)
		return EINVAL;

	attr->scheduler = tid;
	
	return 0;
}
#endif

int
pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
	attr->priority_inherit = PTHREAD_PRIO_NONE;
	
	return 0;
}

int
pthread_mutexattr_destroy(pthread_mutexattr_t *attr)
{
	memset((void *) attr, -1, sizeof(*attr));
	
	return 0;
}

int
pthread_mutexattr_setprotocol(pthread_mutexattr_t *attr, int inherit)
{
	switch (inherit) {
	case PTHREAD_PRIO_NONE:
	case PTHREAD_PRIO_INHERIT:
		attr->priority_inherit = inherit;
		break;

	default:
		return EINVAL;
	}

	return 0;
}

int
pthread_mutexattr_getprotocol(const pthread_mutexattr_t *attr, int *inherit)
{
	*inherit = attr->priority_inherit;

	return 0;
}

int
pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type)
{
	switch (type) {
	case PTHREAD_MUTEX_NORMAL:
	case PTHREAD_MUTEX_ERRORCHECK:
	case PTHREAD_MUTEX_RECURSIVE:
	case PTHREAD_MUTEX_DEFAULT:
		attr->type = type;
		break;

	default:
		return EINVAL;
	}

	return 0;
}

int
pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type)
{
	*type = attr->type;

	return 0;
}

int
pthread_condattr_init(pthread_condattr_t *attr)
{
	return 0;
}

int
pthread_condattr_destroy(pthread_condattr_t *attr)
{
	return 0;
}

