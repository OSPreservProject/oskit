/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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

#include <oskit/time.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/clock.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include "native.h"

#ifdef _OSKIT_C_SYS_TIME_H_
/* oskit doesn't define timespec yet */
#define timespec	oskit_timespec
#define ts_sec		tv_sec
#define ts_nsec		tv_nsec
#endif

/*
 * Initialize the time of day register.
 */
oskit_error_t
oskit_rtc_get(struct oskit_timespec *time)
{
	struct timeval t;
	struct timespec ts;
	int r = NATIVEOS(gettimeofday)(&t, 0);
	/* lets fake local time as MST */
	t.tv_sec -= 6 * 60 * 60;
	TIMEVAL_TO_TIMESPEC(&t, &ts);
	memcpy(time, &ts, sizeof *time);
	return r;
}

/*
 * Write system	time back to RTC
 */
void
oskit_rtc_set(struct oskit_timespec *time)
{
	struct timeval t;
	struct timespec ts;
	memcpy(&ts, time, sizeof ts);
	/* MST to greenwich */
	t.tv_sec += 6 * 60 * 60;
	TIMESPEC_TO_TIMEVAL(&t, &ts);
	NATIVEOS(settimeofday)(&t, 0);
}
