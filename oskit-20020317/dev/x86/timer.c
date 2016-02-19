/*
 * Copyright (c) 1996-1998, 2000, 2001 University of Utah and the Flux Group.
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
 * Timer interrupt support.
 */

#include <oskit/dev/dev.h>
#include <oskit/debug.h>
#include <oskit/x86/pio.h>
#include <oskit/x86/proc_reg.h>
#include "pit_param.h"

#define NTIMERS		10


/* Prototypes for helper functions in another file */
int osenv_timer_pit_init(int freq, void (*timer_intr)());
void osenv_timer_pit_shutdown();
int osenv_timer_pit_read();


struct timer_handler {
	void	(*func)(void);
	struct	timer_handler *next;
};

static struct timer_handler *timer_head, *timer_free;
static struct timer_handler timer_data[NTIMERS];

/*
 * Generic timer interrupt handler.
 */
static void
timer_intr()
{
	struct timer_handler *th;

	for (th = timer_head; th; th = th->next)
		(*th->func)();
}

/*
 * Initialize timers.
 */
void
osenv_timer_init()
{
        int i;
#ifndef KNIT
	static int initialized = 0;  /* guard against multiple invocations */

	if (initialized)
		return;

	initialized = 1;
#endif

	for (i = 0; i < NTIMERS; i++)
		timer_data[i].next = &timer_data[i + 1];
	timer_data[NTIMERS - 1].next = 0;
	timer_free = &timer_data[0];

	/*
	 * Install interrupt handler at TIMER_FREQ Hz.
	 */
	osenv_timer_pit_init(TIMER_FREQ, timer_intr);
}

/*
 * Turn all timers off.
 */
void
osenv_timer_shutdown()
{
	osenv_log(OSENV_LOG_DEBUG, "Shutting down timer\n");
	osenv_timer_pit_shutdown();
}

/*
 * Register a timer handler to be called at the specified frequency.
 */
void
osenv_timer_register(void (*func)(void), int freq)
{
	struct timer_handler *th;
	unsigned int flags;

#ifndef KNIT
	/* XXX */
	osenv_timer_init();
#endif

	if (timer_free == 0)
		osenv_panic("%s:%d: ran out of entries", __FILE__, __LINE__);
	/*
	 * XXX
	 */
	if (freq != TIMER_FREQ)
		osenv_panic("%s:%d: bad freq", __FILE__, __LINE__);
	if (func == 0)
		osenv_panic("%s:%d: null function", __FILE__, __LINE__);

	flags = osenv_intr_save_disable();

	th = timer_free;
	timer_free = th->next;
	th->next = timer_head;
	timer_head = th;
	th->func = func;

	if (flags)
		osenv_intr_enable();
}


/*
 * Frequency and function address uniquely identify which to remove since
 * equal values are equivelent.
 */
void
osenv_timer_unregister(void (*func)(void), int freq)
{
	int flags;
	struct timer_handler *th, **prev;

	if (freq != TIMER_FREQ)
		osenv_panic("%s:%d: bad freq", __FILE__, __LINE__);

	/* dissable interrupts while mucking with the queue */
	flags = osenv_intr_save_disable();

	/* move the timer from the run-queue to the unused-queue */
	prev = &timer_head;
	th = timer_head;

	while (th && (th->func != func)) {
		prev = &th->next;
		th = th->next;
	}
	if (!th) {
		osenv_log(OSENV_LOG_WARNING, "Timer handler not found\n");
	} else {
		/* have the active list skip over it */
		*prev = th->next;

		/* place in inactive queue */
		th->next = timer_free;
		timer_free = th;
	}

	if (flags)
		osenv_intr_enable();
}


/*
 * Spin for a small amount of time, specified in nanoseconds,
 * without blocking or enabling interrupts.
 */
void
osenv_timer_spin(long nanosec)
{
	int prev_val = osenv_timer_pit_read();

	while (nanosec > 0) {
		int val = osenv_timer_pit_read();
		if (val > prev_val)
			prev_val += TIMER_VALUE;
		nanosec -= (prev_val - val) * PIT_NS;
		prev_val = val;
	}
}

/*
 * Return the resolution in nanoseconds of the spin timer.
 */
long
osenv_timer_spin_resolution(void)
{
	return PIT_NS;
}
