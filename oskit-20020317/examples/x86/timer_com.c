/*
 * Copyright (c) 1997-2000 University of Utah and the Flux Group.
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
 * this is an example of how to use timer interrupts with the brand new 
 * COM interfaces
 */
#include <oskit/dev/dev.h>
#include <oskit/time.h>
#include <oskit/dev/clock.h>
#include <oskit/dev/timer.h>
#include <oskit/com/listener.h>
#include <oskit/clientos.h>
#include <oskit/startup.h>
#include <stdio.h>
#include <setjmp.h>
#include <stdlib.h>

/* we don't really need that longjmp */
static jmp_buf done;

/* ARGSUSED */
int handler(struct oskit_iunknown *clock, void *arg)
{
	static sec=0;
	if (++sec > 10) 
		longjmp(done, 1);
	printf("%d second(s)\n", sec);
	return 0;
}

int main(int ac, char *av[])
{
	/* initialize timers */
	oskit_clock_t		*clock; 
	oskit_timer_t		*timer;
	oskit_timespec_t		onesecond = { 1, 0 };
	oskit_itimerspec_t	everysecond = { onesecond, onesecond };

#ifndef KNIT
	oskit_clientos_init();
	/* The clock code needs an osenv */
	start_osenv();
#endif

	/* initialize clock */
	clock = oskit_clock_init();
	/* create a timer */
	oskit_clock_createtimer(clock, &timer);

	/* set a listener */
	oskit_timer_setlistener(timer, oskit_create_listener(handler, 0));

	printf("Counting to ten\n");
	/* start the timer */
	oskit_timer_settime(timer, 0, &everysecond);

	/* wait and exit */
	if (! setjmp(done))
		while (1)
			continue;

	printf("ALL DONE\n");
	exit(0);
}
