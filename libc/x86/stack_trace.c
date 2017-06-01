/*
 * Copyright (c) 1995-1996, 1998, 1999 University of Utah and the Flux Group.
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

#include <stdio.h>

/*
 * This function provides a dump of the function call stack on an x86.  It
 * starts with its caller's frame, and goes back until either the
 * maximum number of levels is exceeded, or its hits a null fp or eip.
 *
 * The prototype is in <oskit/debug.h>
 */

#define MAX_ST_LEVELS 16

/*
 * dump current stack trace to specified printf function
 */
void dump_stack_trace_bis(int (*__printf)(const char *,...), int max_st_levels)
{
	unsigned int *fp, i;
	
	asm volatile ("movl %%ebp,%0" : "=r" (fp));

	__printf("Backtrace: fp=%x", (int) fp);
	for (i = 0; i < max_st_levels; i++) {
		if ((i % 8) == 0)
			__printf("\n");
			
		fp = (int *)(*fp);
		
		if (!(*(fp + 1) && *fp))
			break;

		__printf(" %08x", *(fp + 1));
	}
	__printf("\n");
}

/*
 * dump current stack trace
 */
void dump_stack_trace(void)
{
	dump_stack_trace_bis(printf, MAX_ST_LEVELS);
}

/*
 * store a backtrace
 */
void
store_stack_trace(unsigned *backtrace, int max_len)
{
        unsigned *ebp;
        unsigned i;

        asm volatile("movl %%ebp,%0" : "=r" (ebp));

        for (i = 0; ebp && i < max_len - 1; i++)
        {
		backtrace[i] = ebp[1];
		ebp = (unsigned*)ebp[0];
	}
	backtrace[i] = 0;
}

/*
 * dump a stored backtrace
 */
void 
dump_stored_stack_trace(int (*__printf)(const char *,...), 
		unsigned *backtrace,
		int max_st_levels)
{
	unsigned int i;
	
	__printf("Backtrace:");

	for (i = 0; backtrace[i] && i < max_st_levels; i++)
		__printf(" %08x", backtrace[i]);

	__printf("\n");
}
