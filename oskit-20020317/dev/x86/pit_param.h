/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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
 *  Timer support specific to x86/pc programmable interval timer
 *  This header file contains all parameters
 */

#ifndef _DEV_X86_PIT_PARAM_H_
#define _DEV_X86_PIT_PARAM_H_

#include <oskit/time.h>
#include <oskit/x86/pio.h>
#include <oskit/x86/proc_reg.h>
#include <oskit/x86/pc/pit.h>

/*
 * for now, we have a clock with these parameters
 */
#define TIMER_FREQ	100
#define TIMER_VALUE	(PIT_HZ / TIMER_FREQ)

/* one clock tick is 10M nanoseconds */
#define NANOPERTICK	10000000

#define TIMESPEC2TICKS(t)	((t)->tv_sec * TIMER_FREQ + \
		(((t)->tv_nsec + NANOPERTICK - 1) / NANOPERTICK))

#define TICKS2TIMESPEC(ti, ts) { 				\
	(ts)->tv_sec = (ti)/TIMER_FREQ;				\
	(ts)->tv_nsec = ((ti) % TIMER_FREQ) * NANOPERTICK;	\
    }


#endif /* _DEV_X86_PIT_PARAM_H_ */
