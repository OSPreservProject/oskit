/*
 * Copyright (c) 1995-1996, 1998-2000 University of Utah and the Flux Group.
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
 * Redefined in the pthreads module to give better information.
 */

#include <stdio.h>
#include <stdarg.h>
#include <oskit/debug.h>
#include <stdlib.h>
#include <unistd.h>
#include <threads/pthread_internal.h>

static int paniced;

void		dump_stack_trace_bis(int (*__printf)(const char *,...),
				     int max_st_levels);

/*
 * This function is called by the assert() macro defined in assert.h;
 * it's also a nice simple general-purpose panic function.
 */
void panic(const char *fmt, ...)
{
	pthread_thread_t	*pthread = CURPTHREAD();
	va_list vl;
	int i;

	if (paniced) {
		_exit(2);
	}

	paniced = 1;

	disable_interrupts();
	va_start(vl, fmt);
	vprintf(fmt, vl);
	va_end(vl);

	printf("\n");

	for (i = 0; i < THREADS_MAX_THREAD; i++)
		threads_stack_back_trace(i, 16);

        printf("Tid:%p P:%08x\n", pthread->tid, (int) pthread);

	dump_stack_trace_bis(printf, 32);

	exit(1);
}
