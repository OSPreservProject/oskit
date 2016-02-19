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
 * this is an example of how to use timer interrupts
 * it took me 10 minutes to write
 */
#include <oskit/dev/dev.h>
#include <oskit/c/stdio.h>
#include <oskit/c/stdlib.h>
#include <oskit/clientos.h>
#include <oskit/startup.h>

/* these things should be defined elsewhere, I think */
#define TIMER_FREQ 100
extern void osenv_timer_init();

volatile int sec, looping;

void handler()
{
	static int count = 0;

	if (++count >= 100) {
		count = 0;
		if (looping) {
			++sec;
			printf("%d second(s)\n", sec);
		}
	}
}

int main(int ac, char *av[])
{
#ifndef KNIT
	oskit_clientos_init();
#ifdef GPROF
	start_fs_bmod();
	start_gprof();
#endif
	/* The clock code needs an osenv */
	start_osenv();
	/* initialize timers */
	osenv_timer_init();
#endif

	/* register our handler */
	osenv_timer_register(handler, TIMER_FREQ);

	printf("Counting to ten\n");

	/* wait and exit */
	looping = 1;
	while (sec < 10)
		continue;
	looping = 0;
	printf("ALL DONE\n");
	exit(0);
}
