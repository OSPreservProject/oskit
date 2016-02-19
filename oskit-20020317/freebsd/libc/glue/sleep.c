/*
 * Copyright (c) 1998, 1999 University of Utah and the Flux Group.
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
 * The FreeBSD version of sleep should actually work okay with a little
 * bit of work. Use this for now.
 */

#include <sys/time.h>
#include <errno.h>
#include <oskit/time.h>
#include <oskit/dev/clock.h>
#include <posix/sys/posix.h>

extern oskit_clock_t	*sys_clock;

#ifdef THREAD_SAFE
#include <oskit/threads/pthread.h>

unsigned int
sleep (unsigned int seconds)
{
	/*
	 * Use thread library directly. Put the calling thread to sleep.
	 */
	if (seconds > 0)
		oskit_pthread_sleep(seconds * 1000);
	else
		sched_yield();

	return seconds;
}

void
usleep (unsigned int useconds)
{
	/*
	 * Use thread library directly. Put the calling thread to sleep.
	 * No point in trying to sleep for less than 1 millisecond, since
	 * that is the resolution of pthread_sleep.
	 */
	if (useconds < 1000) {
		sched_yield();
		return;
	}
	
	oskit_pthread_sleep(useconds / 1000);
} 
#else
unsigned int
sleep (unsigned int seconds)
{
	struct oskit_timespec time;

	if (!seconds)
		return 0;

	if (!sys_clock && !posixlib_clock_init())
		return seconds;

	time.tv_nsec = 0;
	time.tv_sec = seconds;

	oskit_clock_spin(sys_clock, &time);

	return 0;
}

void
usleep (unsigned int useconds)
{
	struct oskit_timespec time;

	if (!useconds)
		return;

	if (!sys_clock && !posixlib_clock_init())
		return;

	time.tv_nsec = (useconds % 1000000) * 1000;
	time.tv_sec = useconds / 1000000;

	oskit_clock_spin(sys_clock, &time);

	return;
} 
#endif
