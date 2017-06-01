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
 * Default implementation of osenv_sleep and friends.
 */ 

#include <oskit/debug.h>
#include <oskit/dev/dev.h>

void
osenv_sleep_init(osenv_sleeprec_t *sr) 
{
	osenv_assert(sr);

	sr->data[0] = OSENV_SLEEP_WAKEUP;   /* Return code for osenv_sleep() */
	sr->data[1] = sr;	   /* Just a flag; anything non-null will do */
}

int
osenv_sleep(osenv_sleeprec_t *sr) 
{
	volatile osenv_sleeprec_t *vsr = sr;
	int was_enabled = osenv_intr_enabled();

	osenv_assert(sr);

	/* We won't get anywhere if interrupts aren't enabled! */
	if (!was_enabled)
		osenv_intr_enable();

	/* Busy-wait until osenv_wakeup() clears the flag in the sleeprec */
	while (vsr->data[1])
		/* NOTHING */;

	/* Restore the original interrupt enable state */
	if (!was_enabled)
		osenv_intr_disable();

	return (int) vsr->data[0];
}

void
osenv_wakeup(osenv_sleeprec_t *sr, int wakeup_status) 
{
	osenv_assert(sr);
	sr->data[0] = (void *)wakeup_status;
	sr->data[1] = (void *)0;
}

/*
 * Process locks - the delightfully easy single-threaded version.
 *
 * This is in this file because it is a massive mistake to use mismatched
 * versions of sleep and process_lock and we don't really trust the linker.
 *
 * We do not implement osenv_process_lock_check or osenv_process_lock_locked
 * because they are not used anywhere and aren't mentioned in the 
 * documentation.
 */

/*
 * Lock the process lock.
 */
void
osenv_process_lock(void)
{
}

/*
 * Unlock the process lock.
 */
void
osenv_process_unlock(void)
{
}

/*
 * Test if the current thread has the process lock. Kludge for usermode.
 */
int
osenv_process_locked(void)
{
        /* We _always_ have the lock in single-threaded code. */
        return 1;
}
