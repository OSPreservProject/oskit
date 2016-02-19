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
 * This tests the brand new oskit_clock/oskit_timer COM interfaces some more
 */
#include <oskit/dev/dev.h>
#include <oskit/time.h>
#include <oskit/dev/clock.h>
#include <oskit/dev/timer.h>
#include <oskit/com/listener.h>
#include <oskit/clientos.h>
#include <oskit/startup.h>
#include <stdio.h>
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>

#define TIMERS	6
#define ENDTHIS	30

static jmp_buf 		done;
static oskit_timer_t	*timer[TIMERS];
static oskit_clock_t	*clock;

/*
 * the specs for a bunch of timers
 */
oskit_itimerspec_t	itimers[TIMERS] = 
    { 
	{ {0, 500000000}, {0, 500000000} },	/* every .5 seconds */
	{ {1, 0}, {20, 0} },			/* # will be canceled! */
	{ {1, 0}, {5, 0} },			/* after five, then every sec */
	{ {0, 0}, {3, 0} },			/* after three seconds once */
	{ {2, 0}, {0, 0} },			/* right away, then every two */
	{ {0, 10000000}, {0, 10000000} },	/* every 10 msec */
    };

/*
 * their textual descriptions
 */
char *timer_desc[TIMERS] = {
	"every half a second",
	"SHOULD NOT FIRE!",
	"after five, then every sec",
	"only after three seconds",
	"right away, then every two",
	"#triggered every 10 msec - prints every second",
};

int print_every[TIMERS] = {
	1, 1, 1, 1, 1, 100
};
int count[TIMERS];

int cancel_at[TIMERS] = {
	6, 10, 10, 15, 0, 0	
};
int cancel_or_release[TIMERS] = {
	/* 1 == cancel, 0 == release */
	1,  0,  1,  0, 0, 0	
};
int canceled[TIMERS];


/*
 * The handler that's invoked (obj is the timer fired)
 */
int handler(struct oskit_iunknown *obj, void *arg)
{
	char 			*msg = (char *)arg;
	oskit_timespec_t		curtime;
	int			ti;
	oskit_itimerspec_t	stop = { { 0, 0}, {0, 0} };
	int			print = 0;

	oskit_clock_gettime(clock, &curtime);

	/* find timer index */
	for (ti = 0; ti < TIMERS; ti++) 
		if (timer[ti] == (oskit_timer_t *)obj)
			break;

	assert (ti < TIMERS);

	if (++count[ti] >= print_every[ti]) {
		count[ti] = 0;
		print = 1;
	} 

	if (print) {
		printf("%3d.%03ds, `%s'\n", 
			curtime.tv_sec, curtime.tv_nsec / 1000000, msg);
	}

	/* cancel timer if so programmed */
	for (ti = 0; ti < TIMERS; ti++) 
		if (!canceled[ti] && cancel_at[ti] && 
			curtime.tv_sec >= cancel_at[ti]) 
		{
			oskit_itimerspec_t	left;

			canceled[ti] = 1;
			oskit_timer_gettime(timer[ti], &left);

			if (cancel_or_release[ti]) {
				printf(">>> canceling `%s'\n", timer_desc[ti]);
				oskit_timer_settime(timer[ti], 0, &stop);
			} else {
				printf(">>> releasing `%s'\n", timer_desc[ti]);
				oskit_timer_release(timer[ti]);
				timer[ti] = 0;
			}
			printf(">>> left: %3d.%03ds reload: %3d.%03ds\n",
				    left.it_value.tv_sec,
				    left.it_value.tv_nsec / 1000000,
				    left.it_interval.tv_sec,
				    left.it_interval.tv_nsec / 1000000);
		}

	/* exit after thirty seconds */
	if (curtime.tv_sec > ENDTHIS)
		longjmp(done, 1);

	return 0;
}

int main(int ac, char *av[])
{
	int			i;

#ifndef KNIT
	oskit_clientos_init();
	/* The clock code needs an osenv */
	start_osenv();
#endif

	/* initialize clock */
	clock = oskit_clock_init();

	/* create a bunch of timers timer */
	for (i = 0; i < TIMERS;  i++)
		oskit_clock_createtimer(clock, timer+i);

	/* create a bunch of listeners */
	for (i = 0; i < TIMERS;  i++) {
		oskit_listener_t	*l;

		l = oskit_create_listener(handler, (void *)timer_desc[i]);
		oskit_timer_setlistener(timer[i], l);
		oskit_listener_release(l);
	}

	printf("Starting test\n");

	/* start the timers */
	for (i = 0; i < TIMERS;  i++)
		oskit_timer_settime(timer[i], 0, itimers+i);

	/* wait */
	if (!setjmp(done))
		while (1)
			continue;

	/* release everything */
	printf("releasing COM objects/canceling timers\n");
	for (i = 0; i < TIMERS;  i++)
		if (timer[i])
			oskit_timer_release(timer[i]);
	oskit_clock_release(clock);

	printf("ALL DONE\n");
	exit(0);
}
