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
 * This selectivly overrides half of the default implementation of
 * osenv_timer
 */

#include <oskit/dev/dev.h>

#include "native.h"
#include "support.h"

int
osenv_timer_pit_read()
{
	return 0;
}

#ifdef OSKIT_ARM32
#define osenv_timer_pit_init		oskit_timer_rtc_init
#define osenv_timer_pit_shutdown	oskit_timer_rtc_shutdown
#endif

int
osenv_timer_pit_init(int freq, void (*timer_intr)())
{
	struct itimerval	it;
	memset(&it, 0, sizeof it);
	it.it_value.tv_usec = it.it_interval.tv_usec = 1000000/freq;
	NATIVEOS(setitimer)(ITIMER_VIRTUAL, &it, 0);

	oskitunix_set_signal_handler(SIGVTALRM,
				     (void (*)(int,int,struct sigcontext*))timer_intr);

	return 0;
}

void
osenv_timer_pit_shutdown()
{
	struct itimerval	it;
	memset(&it, 0, sizeof it);
	NATIVEOS(setitimer)(ITIMER_VIRTUAL, &it, 0);
}

/*
 * Helper function for pthreads. Delay by calling select, which avoids
 * cpu spin in the idle loop. If the select timeout, generate a signal
 * initiate a clock tick.
 *
 * N.B. we are operating at the resolution of the underlying unix clock
 * here (10ms) so we have no choice but to sleep for a full ITIMER_VIRTUAL
 * clock tick even though we may be partially or mostly through the current
 * tick.  As a result, you can experience up to 2:1 inflation of virtual to
 * real time in OSKit unix applications that are mostly idle.
 */
void
pthread_delay(void)
{
	struct timeval timeout = { 0, 10000 };

	if (NATIVEOS(select)(0, 0, 0, 0, &timeout) == 0) {
		/*
		 * Timed out. Simulate a clock tick
		 */
		osenv_intr_disable();
		NATIVEOS(kill)(NATIVEOS(getpid)(), SIGVTALRM);
		osenv_intr_enable();
	}
}
