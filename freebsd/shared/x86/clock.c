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

#include <oskit/time.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/clock.h>
#include <oskit/c/stdio.h>

#include <sys/cdefs.h>
#include <sys/time.h>
#include <sys/types.h>
#include "osenv.h"

extern volatile struct timeval time;

int
acquire_timer2(int mode)
{
	return -1;
}

int
release_timer2()
{
	return -1;
}

void DELAY(int usec)
{
	osenv_timer_spin(usec * 1000);
}

int
sysbeep(int pitch, int period)
{
	return -1;
}

/*
 * Initialize the time of day register,	based on the time base which is, e.g.
 * from	a filesystem.
 */
/* ARGSUSED */
int
inittodr(time_t base)
{
	oskit_timespec_t	t;

	osenv_intr_disable();

	/* read real time clock */
	osenv_rtc_get(&t);

	/* t.tv_sec now contains the number of seconds, since Jan 1 1970,
	   in the local	time zone */
#if 0
	/* original BSD code to convert to GMT */
	t.tv_sec += tz.tz_minuteswest * 60 + (wall_cmos_clock ? adjkerntz : 0);
#else
	/* hack for Salt Lake City */
	t.tv_sec += 7 * 60 *60;
#endif
	time.tv_sec = t.tv_sec;
	osenv_intr_enable();
	return 0;
}

/*
 * Write system	time back to RTC
 */
void
resettodr()
{
	oskit_timespec_t t = { 0, 0 };
	oskit_time_t 	tm;

	osenv_intr_disable();
	tm = time.tv_sec;
	osenv_intr_enable();

	/* convert from GMT to localtime */
#if 0
	tm -= tz.tz_minuteswest * 60 + (wall_cmos_clock ? adjkerntz : 0);
#else
	/* hack for Salt Lake City */
	t.tv_sec -= 7 * 60 *60;
#endif

	t.tv_sec = tm;
	osenv_rtc_set(&t);
}
