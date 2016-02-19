/*
 * Copyright (c) 1994-1995, 1998 University of Utah and the Flux Group.
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

#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <oskit/debug.h>

#include "dos_io.h"

int dos_gettimeofday(struct timeval *tv, struct timezone *tz)
{
	static int daysofmonth[12] = {31, 28, 31, 30, 31, 30, 31,
				      31, 30, 31, 30, 31};

	struct trap_state ts;
	int err;

	init_ts(&ts);

	if (tv)
	{
		int year, month, day, hour, min, sec, hund;

		/* Get the current date: year, month, day */
		ts.trapno = 0x21;
		ts.eax = 0x2a00;
		base_real_int(&ts);
		year = ts.ecx & 0xffff;
		month = (ts.edx >> 8) & 0xff;
		day = ts.edx & 0xff;

		/* Get the current time: hour, minute, second, hundredth */
		ts.trapno = 0x21;
		ts.eax = 0x2c00;
		base_real_int(&ts);
		if ((err = dos_check_err(&ts)) != 0)
			return err;
		hour = (ts.ecx >> 8) & 0xff;
		min = ts.ecx & 0xff;
		sec = (ts.edx >> 8) & 0xff;
		hund = ts.edx & 0xff;

		tv->tv_sec = (year - 1970) * (365 * 24 * 60 * 60);
		tv->tv_sec += (year - 1970) / 4 * (24 * 60 * 60); /* XXX??? */
		tv->tv_sec += daysofmonth[month-1] * (24 * 60 * 60);
		if ((((year - 1970) % 4) == 0) && (month > 2)) /* XXX??? */
			tv->tv_sec += 24 * 60 * 60;
		tv->tv_sec += day * 24 * 60 * 60;
		tv->tv_sec += hour * 60 * 60;
		tv->tv_sec += min * 60;
		tv->tv_sec += sec;
		tv->tv_usec = hund * (1000000 / 100);
	}
	if (tz)
		return EINVAL; /*XXX*/

	assert(tz == 0);
	return 0;
}

