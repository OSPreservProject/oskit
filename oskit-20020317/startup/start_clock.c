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
#include <oskit/startup.h>
#include <oskit/time.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/clock.h>
#include <oskit/com/services.h>
#include <oskit/clientos.h>

#include <oskit/c/sys/time.h>
#include <oskit/c/stdlib.h>

long secondswest;
#define LOCAL_TO_GMT(t) (t)->tv_sec += secondswest

void
start_clock()
{
	oskit_timespec_t time;
        oskit_clock_t	*clock;

	/*
	 * The clock code needs part of the osenv. For now, just create
	 * the default osenv. This has the undesirable effect of linking
	 * in more of the device library than is needed for just clocks,
	 * but it is only about 20K extra of text and data.
	 */
	start_osenv();
	clock = oskit_clock_init();

	oskit_rtc_get(&time);
	LOCAL_TO_GMT(&time);
	oskit_clock_settime(clock, &time);

	/*
	 * Register for libraries to grab.
	 */
	if (oskit_register(&oskit_clock_iid, (void *) clock))
		panic("start_clock: clock registration failed");
}
