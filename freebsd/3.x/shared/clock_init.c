/*
 * Copyright (c) 1996, 1998-2001 University of Utah and the Flux Group.
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
#include <sys/time.h>

#include <oskit/dev/dev.h>
#include <time.h>

struct timeval boottime;	/* boottime, referenced by some protocol */

static MALLOC_DEFINE(M_CALLOUT, "callout", "callout queue");

extern void bsd_hardclock(void);
static void timeout_init(void);
extern int ticks;

int ncallout = 256;
struct callout *callout;

static unsigned
dummy_get_timecount(struct timecounter *tc)
{
        static unsigned now;

        return (++now);
}

static struct timecounter dummy_timecounter = {
        dummy_get_timecount,
        0,
        ~0u,
        100,
        "oskit_dummy"
};


void
clockintr(void)
{
	bsd_hardclock();
	setdelayed();
}


/* get clock up and running */
void clock_init()
{
#ifndef KNIT
	osenv_timer_init();
#endif
	timeout_init();
	init_timecounter(&dummy_timecounter);
	inittodr(0);
	osenv_timer_register(clockintr, hz);
}

/* ---------------------------------------------------------------------- */

/*
 * This code comes from sys/i386/i386/machdep.c.
 */
static void 
timeout_init(void)
{
	int i;

        /*
         * Calculate callout wheel size
         */
        for (callwheelsize = 1, callwheelbits = 0;
             callwheelsize < ncallout;
             callwheelsize <<= 1, ++callwheelbits)
                ;
        callwheelmask = callwheelsize - 1;

	/* XXX FreeBSD uses valloc to allocate the callout queue */
	callout = (struct callout *)
		malloc(sizeof(struct callout) * ncallout, M_CALLOUT, M_WAITOK);
	if (!callout)
	        panic("can't allocate callout queue!\n");

	/* XXX FreeBSD uses valloc to allocate the callwheel queue */
	callwheel = (struct callout_tailq *)
		    malloc(sizeof(struct callout_tailq) * callwheelsize,
			   M_CALLOUT, M_WAITOK);
	if (!callwheel)
	        panic("can't allocate callout queue!\n");

	/*
	 * Initialize callouts
	 */
        SLIST_INIT(&callfree);
        for (i = 0; i < ncallout; i++) {
                SLIST_INSERT_HEAD(&callfree, &callout[i], c_links.sle);
        }

        for (i = 0; i < callwheelsize; i++) {
                TAILQ_INIT(&callwheel[i]);
        }
}

/* ---------------------------------------------------------------------- */

void
spin_n_seconds(int n)   
{
        struct timeval mytime, time;

	getmicrotime(&mytime);
	time = mytime;

        while ((time.tv_sec - mytime.tv_sec) < n) {
		getmicrotime(&time);
                continue;
	}
}
 
/* ---------------------------------------------------------------------- */

#if 0
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
#endif

/* ---------------------------------------------------------------------- */
