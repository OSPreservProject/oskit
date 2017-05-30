/*
 * Copyright (c) 2000 University of Utah and the Flux Group.
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
 * `time' based on the OSKit clock interface.
 */

#include <errno.h>
#include <oskit/c/sys/types.h>
#include <oskit/time.h>
#include <oskit/dev/clock.h>

extern oskit_clock_t	*sys_clock;
extern int		posixlib_clock_init(void);

time_t
time(time_t *t)
{
	/* The body of this function looks a lot like the body of function
	   `gettimeofday' in the OSKit's `posix/sys/gettimeofday.c'. */
	
	oskit_timespec_t ts;
	
	if (!sys_clock && !posixlib_clock_init()) {
		errno = ENXIO;
		return ((time_t) -1);
	}
	
	oskit_clock_gettime(sys_clock, &ts);
	if (t)
		*t = ts.tv_sec;
	return ts.tv_sec;
}
