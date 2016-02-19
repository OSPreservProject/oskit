/*
 * Copyright (c) 1996, 1998, 2000 University of Utah and the Flux Group.
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
 * 
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/callout.h>

#include <oskit/dev/dev.h>

struct timeval boottime;	/* boottime, referenced by some protocol */

extern void bsd_hardclock(void);
static void timeout_init(void);

int ncallout = 256;
struct callout *callout;

/* get clock up and running */
void clock_init()
{
#ifndef KNIT
	osenv_timer_init();
#endif
	timeout_init();
	inittodr(0);
	boottime = time;
	osenv_timer_register(bsd_hardclock, hz);
}

/* ---------------------------------------------------------------------- */

static void 
timeout_init(void)
{
	int i;

	callout = (struct callout *)
		malloc(sizeof(struct callout) * ncallout, M_FREE, M_WAITOK);
	if (!callout)
	        panic("can't allocate callout queue!\n");

	/*
	 * Initialize callouts
	 */
	callfree = callout;
	for (i = 1; i < ncallout; i++)
		callout[i-1].c_next = &callout[i];

}

/* ---------------------------------------------------------------------- */

void
spin_n_seconds(int n)   
{
        struct timeval mytime = time;
        while ((time.tv_sec - mytime.tv_sec) < n)
                continue;
}
 
/* ---------------------------------------------------------------------- */

/*
 * normally, microtime accounts for the cycles having passed between
 * the last clock irq and now... I hope we can deal with that weaker
 * monotony - if not, we'll need to include /usr/src/sys/i386/i386/microtime.s
 * and implement all the variables it references in the clock routines
 */
void microtime(struct timeval *tv)
{
	*tv = time;
}

/* ---------------------------------------------------------------------- */
