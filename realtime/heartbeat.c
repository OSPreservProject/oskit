/*
 * Copyright (c) 2000, 2001 University of Utah and the Flux Group.
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
 * A heartbeat and optional panic.
 */
#include <stdio.h>
#include <stdlib.h>
#include <oskit/dev/dev.h>
#include <oskit/base_critical.h>

/* Clock IRQ handlers */
static void		hb_realtime_clock_handler(void *data);
static void		hb_standard_clock_handler(void *data);

/*
 * Clock counts. This allows us to keep track of missed clock
 * interrupts. For every realtime interrupt, there should be a
 * corresponding callback on the standard handler. When interrupts are
 * software disabled, we get callback on the realtime handler, but not
 * on the standard handler. In most cases this one miss will get
 * called later when interrupts are software enabled. However, it is
 * possible to miss more than one if interrupts are disabled a long
 * time. By keeping track of the difference in count, we can schedule
 * enough clock ticks to keep the rest of the oskit time keeping
 * mechanism happy. This essentially operates as an up/down counter.
 */
static unsigned int	clock_ticks;

/*
 * Setup a heartbeat and panic.
 */
void
heartbeat_init()
{
	int		err;

	base_critical_enter();
	
	/*
	 * Add a realtime handler for IRQ 0 (Clock).
	 */
	err = osenv_irq_alloc(0, hb_realtime_clock_handler,
			      0, OSENV_IRQ_REALTIME);
	if (err)
		panic("heartbeat_init: Could not add realtime IRQ\n");

	/*
	 * Add a normal handler for the clock. The override flag allows
	 * a non-shared IRQ to become shared. I prefer this to changing
	 * existing uses of IRQ 0 in other parts of the oskit.
	 */
	err = osenv_irq_alloc(0, hb_standard_clock_handler,
			      0, OSENV_IRQ_OVERRIDE);
	if (err)
		panic("heartbeat_init: Could not add standard IRQ\n");

	base_critical_leave();
}

/*
 * Realtime clock handler. 
 */
static void
hb_realtime_clock_handler(void *data)
{
	clock_ticks++;

	/*
	 * If clock_ticks ever goes above 100, its a sure bet that
	 * something is really wrong!
	 */
	if (clock_ticks > 10)
		panic("Your heart has stopped!");
	
	/*
	 * Always pend a corresponding standard clock IRQ so that the
	 * rest of the oskit stays happy and in time. This will result
	 * in a call to the pthread timer callback.
	 */
	osenv_irq_schedule(0);
}

/*
 * All this does is keep the oskit time in sync by making sure that we
 * get as many standard interrupts handler calls as there are realtime
 * handler calls. 
 */
static void
hb_standard_clock_handler(void *data)
{
	osenv_assert(clock_ticks > 0);

	/*
	 * If multiple missed ticks, schedule another IRQ to be delivered.
	 */
	if (--clock_ticks > 0) {
		osenv_irq_schedule(0);
	}
}
