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
 * Trivial version of sleep based on the oskit clock interface.
 */

#include <unistd.h>
#include <errno.h>
#include <oskit/time.h>
#include <oskit/dev/clock.h>

extern oskit_clock_t	*sys_clock;
extern int		posixlib_clock_init(void);

unsigned int
sleep (unsigned int seconds)
{
	struct oskit_timespec time;

	if (!seconds)
		return 0;

	if (!sys_clock && !posixlib_clock_init())
		return seconds;

	/*
	 * oskit_clock_spin based on osenv_timer_spin, which is limited
	 * to 2^32 nanoseconds.
	 */
	while (seconds) {
		time.tv_nsec = 0;
		time.tv_sec  = 1;

		oskit_clock_spin(sys_clock, &time);

		seconds--;
	}
	return 0;
}

int
usleep (unsigned int useconds)
{
	struct oskit_timespec time;

	if (!useconds)
		return 0;

	if (!sys_clock && !posixlib_clock_init())
		return errno = ENODEV, -1;

	time.tv_nsec = (useconds % 1000000) * 1000;
	time.tv_sec = useconds / 1000000;

	oskit_clock_spin(sys_clock, &time);

	return 0;
} 
