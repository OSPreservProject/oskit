/*
 * Copyright (c) 1997-1999 University of Utah and the Flux Group.
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
 * a simple getitimer/setitimer based on our oskit_clock COM sys_clock
 */
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>

#include <oskit/c/errno.h>

#include <oskit/time.h>
#include <oskit/dev/clock.h>
#include <oskit/com/listener.h>
#include <oskit/dev/timer.h>
#include "posix.h"

static oskit_timer_t	*itimer;
#ifdef KNIT
extern oskit_clock_t	*oskit_system_clock;
#define sys_clock oskit_system_clock
#else
extern oskit_clock_t	*sys_clock;
#endif

#ifdef THREAD_SAFE
/*
 * Deliver the signal to the thread that requested the setitimer. This
 * is probably the wrong thing to do, but sending a kill() also seems wrong
 * since that would cause the alarm to be received in every thread. In fact
 * using setitimer in a multithreaded application seems wrong to begin with.
 */
#include <oskit/threads/pthread.h>

static pthread_t	setitimer_self;
#endif

/* ARGSUSED */
static int
handler(oskit_iunknown_t *obj, void *arg)
{
#ifdef THREAD_SAFE
	return pthread_kill(setitimer_self, SIGALRM);
#else
	return kill(0, SIGALRM);
#endif
}

/*
 * get the interval timer
 */
int 
getitimer(int which, struct itimerval *value)
{
	oskit_itimerspec_t	it;

	if (which != ITIMER_REAL)
		return errno = EINVAL, -1;

	if (!itimer) {
		memset(value, 0, sizeof *value);
		return 0;
	}

	oskit_timer_gettime(itimer, &it);
	TIMESPEC_TO_TIMEVAL(&value->it_interval, &it.it_interval);
	TIMESPEC_TO_TIMEVAL(&value->it_value, &it.it_value);
	return 0;
}

/*
 * set the interval timer
 */
int 
setitimer(int which, struct itimerval *value, struct itimerval *ovalue)
{
	oskit_itimerspec_t	it;

	if (which != ITIMER_REAL)
		return errno = EINVAL, -1;

	if (!itimer) {
		oskit_listener_t	*l;

#ifndef KNIT
		if (!sys_clock && !posixlib_clock_init())
			return errno = ENXIO, -1;
#endif	

		oskit_clock_createtimer(sys_clock, &itimer);
		oskit_timer_setlistener(itimer, 
			l = oskit_create_listener(handler, 0));
		oskit_listener_release(l);
	}
	TIMEVAL_TO_TIMESPEC(&value->it_interval, &it.it_interval) 
	TIMEVAL_TO_TIMESPEC(&value->it_value, &it.it_value)
#ifdef  THREAD_SAFE
	setitimer_self = pthread_self();
#endif
	oskit_timer_settime(itimer, 0, &it);

	return 0;
}

