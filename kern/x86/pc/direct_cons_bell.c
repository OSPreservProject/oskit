/*
 * Copyright (c) 1999 University of Utah and the Flux Group.
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
 * You can ring my bell.
 */

#include <oskit/x86/pc/direct_cons.h>
#include <oskit/x86/pc/speaker.h>
#include <oskit/x86/pc/pit.h>
#include <oskit/x86/pio.h>
#include <oskit/x86/pc/keyboard.h>

#define BELL_FREQ	750	/* Hz */
#define BELL_DURATION	(PIT_HZ * 32) /* ??? I am confused, but this works. */

void
direct_cons_bell(void)
{
	unsigned int elapsed, val, lastval;

	pc_speaker_on(BELL_FREQ);

	/*
	 * Spin until we hit the timeout.
	 * We use the PIT directly here instead of osenv_timer_spin,
	 * to avoid any link-time dependencies.  Any real user can
	 * override direct_cons_bell with a version that sleeps properly.
	 * Since we can't rely on the real time clock (timer 0) having
	 * been set up yet, we just read the very clock value that is
	 * driving the speaker.
	 */
	elapsed = 0;
	lastval = pit_read(2);
	do {
		val = pit_read(2);
		if (val > lastval) /* the countdown timer fired and reset */
			lastval += PIT_HZ / BELL_FREQ;
		elapsed += lastval - val;
	} while (elapsed < BELL_DURATION);

	pc_speaker_off();
}
