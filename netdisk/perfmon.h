/*
 * Copyright (c) 1996-2001 University of Utah and the Flux Group.
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

#ifdef DOTIMEIT
#include "timer.h"

typedef unsigned long	tstamp_t;

#define GETTSTAMP(ts)	(ts) = currticks()
#define SUBTSTAMP(s, t) ((s) -= (t))

#if TIMER_FREQ == 100
#define PRINTTSTAMP(ts) printf(" in %ld.%02ld seconds", (ts)/100, (ts)%100)
#else 
#define PRINTTSTAMP(ts)	printf(" in %d ticks", (ts))
#endif
#else
#define GETTSTAMP(ts)
#define SUBTSTAMP(s, t)
#define PRINTTSTAMP(ts)	
#endif

#ifdef CYCLECOUNTS
#include <oskit/machine/proc_reg.h>

#define GETCSTAMP(cs)		(cs) = get_tsc();
#define UPDATECSTAMP(tot, cs)   ((tot) += (get_tsc() - (cs)))

extern long long total_readwait_cycles;
extern long long total_sleep_cycles;
#else
#define GETCSTAMP(cs)
#define UPDATECSTAMP(tot, cs)	
#endif

