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
 * This contains helper functions for the fdev_timer interface.
 * This is in a separate file for the convenience of the Unix code
 * so it only has to override half the fdev_timer support.
 */

#include <oskit/dev/dev.h>
#include <oskit/debug.h>
#include <oskit/x86/pio.h>
#include <oskit/x86/proc_reg.h>
#include <oskit/x86/pc/pit.h>

static void(*handler)();

int
osenv_timer_pit_init(int freq, void (*timer_intr)())
{
        oskit_error_t rc;

	/*
	 * Initialize timer to interrupt at TIMER_FREQ Hz.
	 */
	pit_init(freq);

	/*
	 * Install interrupt handler.
	 */
	if ((rc = osenv_irq_alloc(0, timer_intr, 0, 0)))
		osenv_panic(__FUNCTION__": couldn't install intr handler %x", rc);
	handler = timer_intr;

	return freq;
}


void
osenv_timer_pit_shutdown()
{
	osenv_irq_free(0, handler, 0);
}


int
osenv_timer_pit_read()
{
	int enb;
	int value;

	enb = osenv_intr_save_disable();
	value = pit_read(0);
	if (enb)
		osenv_intr_enable();
	return value;
}
