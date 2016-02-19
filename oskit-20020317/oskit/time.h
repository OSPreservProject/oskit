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
 * Basic time-related type definitions.
 */
#ifndef _OSKIT_TIME_H_
#define _OSKIT_TIME_H_

#include <oskit/types.h>

/*
 * Type representing a time in seconds since the Epoch
 * (midnight, January 1, 1970, GMT).
 * XXX put this in machine/types.h?  netbsd/alpha uses 64 bits.
 */
typedef oskit_s32_t	oskit_time_t;

/*
 * This is the standard high-resolution time structure for OSKit interfaces;
 * it corresponds directly to POSIX's timespec structure.
 */
struct oskit_timespec {
	oskit_time_t	tv_sec;		/* Seconds */
	oskit_s32_t	tv_nsec;	/* Nanoseconds */
};
typedef struct oskit_timespec oskit_timespec_t;

/*
 * Timer expiration and period specification structure,
 * corresponding to POSIX's itimerspec structure.
 */
struct oskit_itimerspec {
	oskit_timespec_t	it_interval;	/* Timer period */
	oskit_timespec_t	it_value;	/* Timer expiration */
};
typedef struct oskit_itimerspec oskit_itimerspec_t;

#include <oskit/compiler.h>

#endif /* _OSKIT_TIME_H_ */
