/*
 * Copyright (c) 2001 The University of Utah and the Flux Group.
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

#include <oskit/c/environment.h>
#include <oskit/c/stdarg.h>
#include <oskit/dev/clock.h>
#include "oskit_uvm_internal.h"

/* from machdep.c */
int	physmem;
int	cpu_class;
int	cpu_feature;

/* from init_main.c */
struct	vnode *rootvp, *swapdev_vp;
int	cold = 0;

/* from vfs_bio.c */
struct	pool bufpool;

/* from param.c */
#ifndef HZ
#define HZ 100
#endif
int	hz = HZ;

/*
 *  this function is an alternative for mono_time variable.
 *  We (re)define mono_time in <sys/kernel.h> as follows.
 *  #define mono_time ({struct timeval x; uvm_gettimeofday(&x); x;})
 */
/* ARGSUSED */
extern void
uvm_gettimeofday(struct timeval *tvp)
{
    extern oskit_clock_t	*sys_clock;
    oskit_timespec_t		ts;

    if (!sys_clock) {
	oskit_library_services_lookup(&oskit_clock_iid,
				      (void *) &sys_clock);
	if (sys_clock == 0) {
	    panic(__FUNCTION__": UVM needs sys_clock\n");
	}
    }
    
    oskit_clock_gettime(sys_clock, &ts);
    TIMESPEC_TO_TIMEVAL(tvp, &ts);
}

/* No dumppage for the OSKit */
extern vaddr_t
reserve_dumppages(vaddr_t p)
{
    return p;
}

extern void
microtime(struct timeval *tvp)
{
    /* Right thing to do? */
    uvm_gettimeofday(tvp);
    while (tvp->tv_usec > 1000000)
    {
	tvp->tv_sec++;
	tvp->tv_usec -= 1000000;
    }
}
